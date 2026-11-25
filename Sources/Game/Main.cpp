// SPDX-License-Identifier: MIT
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <optional>
#include <vector>

#include "Game/Apartment.hpp"
#include "Game/CaseAudio.hpp"
#include "Game/CasePanel.hpp"
#include "Game/CaseProps.hpp"
#include "Game/Dialog.hpp"
#include "Game/SettingsMenu.hpp"
#include "Investigation/Case.hpp"

namespace TwentyThreeSeventeen::Game
{
    namespace
    {
        using Investigation::CaseId;
        using Investigation::Subject;
        constexpr std::array kCaseIds{ CaseId::kApartment,
                                       CaseId::kArchive,
                                       CaseId::kCinema };
        using Rendering::Backend;
        using Rendering::Renderer;
        using Rendering::WindowSize;
        using Timer = std::chrono::steady_clock;

        struct HudText
        {
            const char *complete;
            const char *filed;
            const char *controls;
        };

        constexpr std::array<HudText, 3> kHudText{ {
            { "Investigation ready for a report",
              "Report filed",
              "WASD Move | E Inspect | J Journal | R Report | Esc Pause" },
            { "Fall bereit für den Bericht",
              "Bericht eingereicht",
              "WASD Gehen | E Untersuchen | J Akte | R Bericht | Esc Pause" },
            { "Enquête prête pour le rapport",
              "Rapport remis",
              "WASD Déplacement | E Examiner | J Carnet | R Rapport | Échap "
              "Pause" },
        } };

        struct HorrorSequence
        {
            float time{ -1.0f };
            float captionTime{};
            int steps{};
            bool returnRing{};
            Sound cue{};

            auto
            Advance(CaseId id,
                    CaseAudio &audio,
                    Physics::Position eye,
                    float zone,
                    float elapsed,
                    bool stopped) -> void
            {
                captionTime = std::max(0.0f, captionTime - elapsed);
                const auto emit =
                    [&](Sound sound, Physics::Position point, float duration) {
                        audio.Cue(sound, point);
                        cue = sound;
                        captionTime = duration;
                    };
                if (time < 0.0f)
                {
                    if (std::abs(eye.z - zone) >= 0.85f)
                        return;
                    time = 0.0f;
                    if (id == CaseId::kArchive)
                        emit(Sound::kTelephone, { -1.45f, 1.2f, -6.0f }, 2.7f);
                }
                const auto previous = time;
                time = std::min(8.0f, time + elapsed);
                if (id == CaseId::kApartment && stopped
                    && std::abs(eye.z - zone) < 2.1f && steps < 3
                    && time > 0.85f + 0.58f * static_cast<float>(steps))
                {
                    ++steps;
                    const auto offset = static_cast<float>(steps) * 0.51f;
                    emit(Sound::kFootstep,
                         { 0.7f, eye.y - 1.3f, eye.z + 2.1f - offset },
                         2.2f);
                }
                if (id == CaseId::kArchive && !returnRing && time > 2.6f
                    && eye.z > 0.5f)
                {
                    returnRing = true;
                    emit(Sound::kTelephone, { -1.45f, 1.2f, 1.65f }, 2.7f);
                }
                if (id == CaseId::kCinema && previous < 1.4f && time >= 1.4f)
                    emit(Sound::kProjector, { 0, 2.7f, 0.08f }, 3.7f);
            }

            [[nodiscard]] auto
            Caption(Language language) const noexcept -> std::string_view
            {
                constexpr std::array captions{
                    std::array{ "", "", "" },
                    std::array{ "[Footsteps on the stairs]",
                                "[Schritte auf der Treppe]",
                                "[Des pas dans l’escalier]" },
                    std::array{ "[A telephone rings]",
                                "[Ein Telefon klingelt]",
                                "[Un téléphone sonne]" },
                    std::array{ "[The projector starts]",
                                "[Der Projektor läuft an]",
                                "[Le projecteur démarre]" },
                };
                return captionTime > 0.0f
                           ? captions[static_cast<std::size_t>(cue)]
                                     [static_cast<std::size_t>(language)]
                           : "";
            }
        };

        struct Options
        {
            int resolutionOverride{ -1 };
            Backend override{ Backend::kAuto };
            bool hasOverride{};
            bool disableSync{};
            bool verifyRoute{};
            bool verifySwitch{};
            int frames{};
            int benchmarkSeconds{};
            std::optional<CaseId> caseId;
            bool verifyCampaign{};
            bool fullscreen{};
        };

        struct Graphics
        {
            std::unique_ptr<Renderer> renderer;
        };

        auto
        LocalLanguage() -> Language
        {
            int count = 0;
            const auto locales = SDL_GetPreferredLocales(&count);
            const auto *language = (count > 0) ? locales[0]->language : "";
            const auto key = std::string_view(language);
            const auto selected = key == "de"   ? Language::kGerman
                                  : key == "fr" ? Language::kFrench
                                                : Language::kEnglish;
            SDL_free(locales);
            return selected;
        }

        auto
        ReadOptions(std::span<char *> arguments)
            -> tl::expected<Options, std::string>
        {
            Options options;
            for (const auto *argument : arguments)
            {
                const std::string_view value(argument);
                if (value == "--no-vsync")
                    options.disableSync = true;
                else if (value == "--fullscreen")
                    options.fullscreen = true;
                else if (value == "--verify-route")
                    options.verifyRoute = true;
                else if (value == "--verify-campaign")
                    options.verifyCampaign = true;
                else if (value.starts_with("--case="))
                {
                    options.caseId.reset();
                    for (const auto id : kCaseIds)
                        if (value.substr(7)
                            == Investigation::Definition(id).key)
                            options.caseId = id;
                    if (!options.caseId)
                        return tl::unexpected(std::string("Unknown case."));
                }
                else if (value == "--verify-switch")
                    options.verifySwitch = true;
                else if (value.starts_with("--graphics="))
                {
                    const auto key = value.substr(11);
                    options.override = Rendering::ParseBackend(key);
                    if (key != Rendering::Key(options.override))
                        return tl::unexpected(
                            std::string("Expected auto, vulkan or opengl."));
                    options.hasOverride = true;
                }
                else if (value.starts_with("--frames="))
                {
                    const auto parsed
                        = std::from_chars(value.data() + 9,
                                          value.data() + value.size(),
                                          options.frames);
                    if (parsed.ec != std::errc()
                        || parsed.ptr != value.data() + value.size()
                        || options.frames <= 0)
                        return tl::unexpected(
                            std::string("Frame count must be positive."));
                }
                else if (value.starts_with("--benchmark-seconds="))
                {
                    const auto duration = value.substr(20);
                    const auto parsed
                        = std::from_chars(duration.data(),
                                          duration.data() + duration.size(),
                                          options.benchmarkSeconds);
                    if (parsed.ec != std::errc()
                        || parsed.ptr != duration.data() + duration.size()
                        || options.benchmarkSeconds < 1
                        || options.benchmarkSeconds > 600)
                        return tl::unexpected(std::string(
                            "Benchmark duration must be 1 to 600 seconds."));
                }
                else if (value == "--1280x720")
                    options.resolutionOverride = 1;
                else if (value == "--1366x768")
                    options.resolutionOverride = 2;
                else
                    return tl::unexpected(std::string("Unknown option: ")
                                          + argument);
            }
            if (options.benchmarkSeconds > 0)
            {
                if (options.frames > 0)
                    return tl::unexpected(std::string(
                        "Choose a frame count or a benchmark duration."));
                options.disableSync = true;
                if (options.resolutionOverride < 0)
                    options.resolutionOverride = 1;
            }
            return options;
        }

        auto
        SettingsFile() -> std::filesystem::path
        {
            auto *path = SDL_GetPrefPath("23.17", "Engine");
            if (path == nullptr)
                return {};
            auto file = std::filesystem::path(std::u8string(
                            reinterpret_cast<const char8_t *>(path)))
                        / "graphics.txt";
            SDL_free(path);
            return file;
        }

        auto
        ReadSettings(const std::filesystem::path &path) -> Settings
        {
            std::ifstream stream(path);
            std::string stored;
            std::getline(stream, stored);
            return LoadSettings(stored, LocalLanguage());
        }

        auto
        DesiredSize(const Settings &settings) noexcept -> WindowSize
        {
            return kResolutions[static_cast<std::size_t>(settings.resolution)];
        }

        auto
        WriteLine(const std::filesystem::path &path, std::string_view value)
            -> bool
        {
            if (path.empty())
                return false;
            auto temporary = path;
            temporary
                += "." + std::to_string(SDL_GetPerformanceCounter()) + ".tmp";
            std::ofstream stream(temporary, std::ios::trunc);
            stream << value << '\n';
            stream.close();
            std::error_code error;
            if (stream)
            {
                std::filesystem::rename(temporary, path, error);
                if (!error)
                    return true;
            }
            std::filesystem::remove(temporary, error);
            return false;
        }

        auto
        Choose(Backend choice, WindowSize size, bool sync, bool shouldFallback)
            -> tl::expected<std::unique_ptr<Renderer>, std::string>
        {
            if (choice != Backend::kAuto && !shouldFallback)
                return Rendering::Open(choice, size, sync);

            std::string errors;
            for (const auto backend : Rendering::StartupOrder(choice))
            {
                auto result = Rendering::Open(backend, size, sync);
                if (result)
                    return result;
                SDL_Log("%s unavailable: %s",
                        Rendering::Name(backend).data(),
                        result.error().c_str());
                if (!errors.empty())
                    errors += '\n';
                errors += std::string(Rendering::Name(backend)) + ": "
                          + result.error();
            }
            return tl::unexpected(errors);
        }

        auto
        BuildVersion() -> std::string
        {
            const auto *base = SDL_GetBasePath();
            if (base == nullptr)
                return {};
            const auto location
                = std::filesystem::path(reinterpret_cast<const char8_t *>(base))
                  / "VERSION.txt";
            std::ifstream input(location);
            std::string version;
            if (std::getline(input, version) && version.size() <= 64
                && version.find_first_not_of("0123456789abcdef.+")
                       == std::string::npos)
                return version;
            return {};
        }

        auto
        UseWindow(Graphics &graphics) -> void
        {
            auto *window = graphics.renderer->Window();
            if (const auto version = BuildVersion(); !version.empty())
                SDL_SetWindowTitle(window, ("23.17 " + version).c_str());
            SDL_ShowWindow(window);
            SDL_SetWindowRelativeMouseMode(window, true);
        }

        auto
        ConfigureWindow(SDL_Window *window, const Settings &settings)
            -> tl::expected<void, std::string>
        {
            const auto fullscreen
                = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
            if (settings.fullscreen != fullscreen
                && !SDL_SetWindowFullscreen(window, settings.fullscreen))
                return tl::unexpected(std::string(SDL_GetError()));
            if (!settings.fullscreen)
            {
                const auto size = DesiredSize(settings);
                if (!SDL_SetWindowSize(window, size.width, size.height))
                    return tl::unexpected(std::string(SDL_GetError()));
            }
            return {};
        }

        auto
        ApplySettings(Graphics &graphics,
                      Settings &current,
                      const Settings &candidate,
                      const std::filesystem::path &path)
            -> tl::expected<void, std::string>
        {
            if (candidate == current)
                return {};

            const bool changeBackend = current.backend != candidate.backend;
            const bool replaceRenderer
                = changeBackend || current.sync != candidate.sync;
            const bool changeWindow
                = current.resolution != candidate.resolution
                  || current.fullscreen != candidate.fullscreen;
            std::unique_ptr<Renderer> replacement;
            if (replaceRenderer)
            {
                const auto requested = changeBackend
                                           ? candidate.backend
                                           : graphics.renderer->ActiveBackend();
                auto opened = Choose(requested,
                                     DesiredSize(candidate),
                                     candidate.sync,
                                     false);
                if (!opened)
                    return tl::unexpected(opened.error());
                replacement = std::move(*opened);
            }

            auto *window = replacement ? replacement->Window()
                                       : graphics.renderer->Window();
            if (replaceRenderer || changeWindow)
            {
                if (auto configured = ConfigureWindow(window, candidate);
                    !configured)
                {
                    if (!replacement)
                    {
                        const auto restored = ConfigureWindow(window, current);
                        if (!restored)
                            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                        "%s",
                                        restored.error().c_str());
                    }
                    return tl::unexpected(configured.error());
                }
            }

            if (!WriteLine(path, SerializeSettings(candidate)))
            {
                if (changeWindow && !replacement)
                {
                    const auto restored = ConfigureWindow(window, current);
                    if (!restored)
                        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                    "%s",
                                    restored.error().c_str());
                }
                return tl::unexpected(
                    std::string("Could not save graphics.txt."));
            }

            if (replacement)
            {
                graphics.renderer = std::move(replacement);
                SDL_ShowWindow(graphics.renderer->Window());
            }
            current = candidate;
            return {};
        }

        auto
        SettingsDialog(Graphics &graphics,
                       Settings &settings,
                       const std::filesystem::path &path) -> bool
        {
            SDL_SetWindowRelativeMouseMode(graphics.renderer->Window(), false);
            while (true)
            {
                auto draft = settings;
                auto *window = graphics.renderer->Window();
                WindowSize actual{};
                SDL_GetWindowSize(window, &actual.width, &actual.height);
                Dialog dialog(*graphics.renderer);
                const auto apply
                    = EditSettings(dialog,
                                   draft,
                                   graphics.renderer->ActiveBackend(),
                                   actual);
                if (dialog.WantsQuit())
                    return false;
                if (!apply)
                    return true;
                const auto result
                    = ApplySettings(graphics, settings, draft, path);
                if (result)
                    return true;
                Dialog error(*graphics.renderer);
                ShowSettingsError(error, settings.language, result.error());
                if (error.WantsQuit())
                    return false;
            }
        }

        auto
        RunMenu(Graphics &graphics,
                Settings &settings,
                const std::filesystem::path &path) -> MenuAction
        {
            SDL_SetWindowRelativeMouseMode(graphics.renderer->Window(), false);
            while (true)
            {
                Dialog dialog(*graphics.renderer);
                switch (StartMenu(dialog, settings.language))
                {
                    case MenuAction::kContinue:
                        SDL_SetWindowRelativeMouseMode(
                            graphics.renderer->Window(),
                            true);
                        return MenuAction::kContinue;
                    case MenuAction::kCaseList:
                        return MenuAction::kCaseList;
                    case MenuAction::kQuit:
                        return MenuAction::kQuit;
                    case MenuAction::kSettings:
                        if (!SettingsDialog(graphics, settings, path))
                            return MenuAction::kQuit;
                        break;
                }
            }
        }

        auto
        LoadCase(const std::filesystem::path &path, CaseId id)
            -> Investigation::Case
        {
            std::ifstream stream(path);
            std::string stored;
            if (!std::getline(stream, stored))
                return Investigation::Case(id);

            const auto restored = Investigation::Case::Parse(stored);
            if (!restored)
            {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "%s",
                            restored.error().data());
                return Investigation::Case(id);
            }
            if (restored->Id() != id)
            {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "Save belongs to another case.");
                return Investigation::Case(id);
            }
            return *restored;
        }

        auto
        Aim(Physics::Position eye, Physics::Position target)
            -> Physics::Position
        {
            const auto dx = target.x - eye.x;
            const auto dy = target.y - eye.y;
            const auto dz = target.z - eye.z;
            const auto distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            return { dx / distance, dy / distance, dz / distance };
        }

        auto
        VerifyRoute(CaseId id) -> bool
        {
            using Investigation::ChallengeResult;
            using Investigation::Claim;
            const auto scene = CreateScene(id);
            Physics::Player player(scene.obstacles);
            Investigation::Case investigation(id);
            constexpr auto step = 1.0f / 120.0f;
            constexpr float walkSpeed = 3.05f;
            const auto walk = [&](int frames) {
                for (int frame = 0; frame < frames; ++frame)
                    player.Move(0, -walkSpeed, step);
            };
            const auto observe = [&](Subject subject) {
                const auto match = std::ranges::find_if(
                    scene.obstacles,
                    [subject](const auto &obstacle) {
                        return obstacle.subject
                               == static_cast<std::uint64_t>(subject);
                    });
                if (match == scene.obstacles.end())
                    return;
                const auto seen = static_cast<Subject>(
                    player.LookAt(Aim(player.Eye(), match->center)));
                if (seen == subject)
                    investigation.Observe(subject);
                else
                    SDL_Log(
                        "Ray contact %u (expected %u at eye %.2f, %.2f, %.2f)",
                        static_cast<unsigned>(seen),
                        static_cast<unsigned>(subject),
                        static_cast<double>(player.Eye().x),
                        static_cast<double>(player.Eye().y),
                        static_cast<double>(player.Eye().z));
            };
            for (int frame = 0; frame < 120; ++frame)
                player.Move(0, 0, step);
            if (id == CaseId::kApartment)
            {
                observe(Subject::kDispatch);
                observe(Subject::kCamera);
                walk(138);
                observe(Subject::kDoorLog);
                walk(133);
                observe(Subject::kRecorder);
                walk(234);
                observe(Subject::kBreaker);
                observe(Subject::kElectrician);
                observe(Subject::kCaretaker);
                observe(Subject::kRevision);
            }
            else if (id == CaseId::kArchive)
            {
                observe(Subject::kInventory);
                observe(Subject::kArchivist);
                walk(170);
                observe(Subject::kTransfer);
                walk(135);
                observe(Subject::kShelf);
                walk(135);
                observe(Subject::kDuty);
                walk(140);
                observe(Subject::kIntake);
                observe(Subject::kReceiver);
            }
            else
            {
                observe(Subject::kSchedule);
                observe(Subject::kUsher);
                walk(150);
                observe(Subject::kScreening);
                observe(Subject::kDiary);
                walk(400);
                observe(Subject::kLeader);
                walk(110);
                observe(Subject::kExit);
                observe(Subject::kReceipt);
            }
            const auto &definition = Investigation::Definition(id);
            if (!investigation.CompareRecords(definition.comparison[0],
                                              definition.comparison[1]))
                return false;
            for (std::size_t index = 0; index < definition.claims.size();
                 ++index)
                if (investigation.Challenge(static_cast<Claim>(index),
                                            definition.claims[index].record)
                    != ChallengeResult::kClarified)
                    return false;
            investigation.MoveEvent(0, 1);
            investigation.MoveEvent(3, -1);
            investigation.MoveEvent(2, -1);
            for (std::size_t index = 0;
                 index < definition.supportedAnswers.size();
                 ++index)
                investigation.SetAnswer(index,
                                        definition.supportedAnswers[index]);
            if (investigation.Submit()
                != Investigation::ReportFinding::kSupported)
                return false;
            const auto settingsPath = SettingsFile();
            if (settingsPath.empty())
                return false;
            const auto testPath
                = settingsPath.parent_path()
                  / ("verify-case-"
                     + std::to_string(SDL_GetPerformanceCounter()) + ".txt");
            const bool written
                = WriteLine(testPath, Investigation::Case(id).Serialize())
                  && WriteLine(testPath, investigation.Serialize());
            const auto restored = LoadCase(testPath, id);
            std::error_code error;
            std::filesystem::remove(testPath, error);
            if (written && restored.Filed()
                && restored.Serialize() == investigation.Serialize())
            {
                SDL_Log("Verified %s: all sources, two challenges, timeline, "
                        "report and save.",
                        definition.key.data());
                return true;
            }
            return false;
        }

        auto
        CaseFile(const std::filesystem::path &settingsPath, CaseId id)
            -> std::filesystem::path
        {
            if (settingsPath.empty())
                return {};
            return settingsPath.parent_path()
                   / ("case-" + std::string(Investigation::Definition(id).key)
                      + ".txt");
        }

        auto
        SelectCase(Graphics &graphics,
                   Settings &settings,
                   const std::filesystem::path &path) -> std::optional<CaseId>
        {
            constexpr std::array titles{ "Case files",
                                         "Fallakten",
                                         "Dossiers" };
            constexpr std::array descriptions{
                "Choose any case. Each file keeps its own progress.",
                "Wähle einen beliebigen Fall. Jede Akte speichert ihren "
                "eigenen Stand.",
                "Choisis une affaire. Chaque dossier conserve sa propre "
                "progression."
            };
            constexpr std::array settingsLabels{ "Settings",
                                                 "Einstellungen",
                                                 "Paramètres" };
            constexpr std::array quits{ "Quit", "Beenden", "Quitter" };
            constexpr std::array openLabels{ "Open case",
                                             "Fall öffnen",
                                             "Ouvrir le dossier" };
            constexpr std::array back{ "Back", "Zurück", "Retour" };
            constexpr std::array working{ "  [In progress]",
                                          "  [In Arbeit]",
                                          "  [En cours]" };
            constexpr std::array closed{ "  [Filed]",
                                         "  [Abgeschlossen]",
                                         "  [Classé]" };
            SDL_SetWindowRelativeMouseMode(graphics.renderer->Window(), false);
            while (true)
            {
                const auto language
                    = static_cast<std::size_t>(settings.language);
                std::array<std::string, 3> names;
                for (std::size_t index = 0; index < kCaseIds.size(); ++index)
                {
                    const auto id = kCaseIds[index];
                    const auto state = LoadCase(CaseFile(path, id), id);
                    names[index] = CasePanel(settings.language, id).Title();
                    if (state.Filed())
                        names[index] += closed[language];
                    else if (state.HasProgress())
                        names[index] += working[language];
                }
                const std::array choices{ names[0].c_str(),
                                          names[1].c_str(),
                                          names[2].c_str(),
                                          settingsLabels[language] };
                Dialog dialog(*graphics.renderer);
                const auto choice = dialog.Choose(titles[language],
                                                  descriptions[language],
                                                  quits[language],
                                                  choices);
                if (dialog.WantsQuit() || choice < 0)
                    return std::nullopt;
                if (choice == 3)
                {
                    if (!SettingsDialog(graphics, settings, path))
                        return std::nullopt;
                    continue;
                }
                const auto id = kCaseIds[static_cast<std::size_t>(choice)];
                const CasePanel panel(settings.language, id);
                const std::array open{ openLabels[language] };
                const auto began = dialog.Choose(panel.Title(),
                                                 panel.Brief(),
                                                 back[language],
                                                 open);
                if (dialog.WantsQuit())
                    return std::nullopt;
                if (began >= 0)
                    return id;
            }
        }

        auto
        PlayCase(const Options &options,
                 Graphics &graphics,
                 CaseAudio &audio,
                 Settings &settings,
                 const std::filesystem::path &path,
                 CaseId id,
                 bool &caseList,
                 const char *executable) -> int
        {
            auto apartment = CreateScene(id);
            AddCaseProps(apartment, id, executable);
            auto materials = Rendering::LoadMaterials(executable);
            if (!materials)
            {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                             "%s",
                             materials.error().c_str());
                return EXIT_FAILURE;
            }
            const auto world = BuildWorld(apartment, std::move(*materials));
            const auto casePath = CaseFile(path, id);
            auto investigation = LoadCase(casePath, id);
            std::error_code existenceError;
            if (!path.empty() && id == CaseId::kApartment
                && !std::filesystem::exists(casePath, existenceError)
                && !existenceError)
            {
                investigation
                    = LoadCase(path.parent_path() / "case-progress.txt", id);
                if (investigation.HasProgress())
                    WriteLine(casePath, investigation.Serialize());
            }
            const auto checkpointSource
                = id == CaseId::kApartment ? Subject::kRevision
                  : id == CaseId::kArchive ? Subject::kIntake
                                           : Subject::kExit;
            constexpr Physics::Position entrance{ 0, 0.08f, 3.9f };
            const bool benchmarking = options.benchmarkSeconds > 0;
            const bool measuring = benchmarking || options.frames > 0;
            Physics::Player player(apartment.obstacles,
                                   investigation.HasObserved(checkpointSource)
                                           && !measuring
                                       ? apartment.checkpoint
                                       : entrance);
            SDL_SetWindowRelativeMouseMode(graphics.renderer->Window(),
                                           !measuring);
            CasePanel casePanel(settings.language, id);
            std::vector<Rendering::Vertex> vertices;
            vertices.reserve(apartment.surfaces.size() * 36 + 12);
            const auto started = Timer::now();
            const auto measureFrom = started + std::chrono::seconds(2);
            const auto until
                = measureFrom + std::chrono::seconds(options.benchmarkSeconds);
            const auto benchmarkSize = DesiredSize(settings);
            bool presented = true;
            auto previous = started;
            int frames = 0;
            float yaw = 0;
            float pitch = 0;
            float pending = 0;
            HorrorSequence horror;
            float benchmarkDirection = 1.0f;
            Subject previousTarget = Subject::kNone;
            bool refreshTitle = true;
            bool running = true;
            std::vector<double> samples;
            if (options.frames > 0)
                samples.reserve(static_cast<std::size_t>(options.frames));
            while (running && (options.frames == 0 || frames < options.frames))
            {
                const auto now = Timer::now();
                const auto elapsed
                    = std::chrono::duration<float>(now - previous).count();
                previous = now;
                if (benchmarking && now >= until)
                    break;
                if ((!benchmarking && options.frames > 0 && frames > 10)
                    || (benchmarking && previous >= measureFrom))
                    samples.push_back(static_cast<double>(elapsed) * 1000.0);
                pending += std::min(elapsed, 0.06f);

                bool inspect = false;
                bool openJournal = false;
                bool openReport = false;
                SDL_Event event;
                while (SDL_PollEvent(&event))
                {
                    if (event.type == SDL_EVENT_QUIT)
                        running = false;
                    if (benchmarking)
                        continue;
                    if (event.type == SDL_EVENT_MOUSE_MOTION)
                    {
                        const auto sensitivity
                            = kSensitivities[static_cast<std::size_t>(
                                settings.sensitivity)];
                        const auto vertical
                            = settings.invertMouse ? sensitivity : -sensitivity;
                        yaw += event.motion.xrel * sensitivity;
                        pitch = std::clamp(pitch + event.motion.yrel * vertical,
                                           -1.48f,
                                           1.48f);
                    }
                    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
                    {
                        if (event.key.scancode == SDL_SCANCODE_ESCAPE
                            || event.key.scancode == SDL_SCANCODE_F1)
                        {
                            audio.Silence();
                            if (event.key.scancode == SDL_SCANCODE_F1)
                            {
                                running
                                    = SettingsDialog(graphics, settings, path);
                                if (running)
                                    SDL_SetWindowRelativeMouseMode(
                                        graphics.renderer->Window(),
                                        true);
                            }
                            else
                            {
                                const auto action
                                    = RunMenu(graphics, settings, path);
                                caseList = action == MenuAction::kCaseList;
                                running = action == MenuAction::kContinue;
                            }
                            refreshTitle = true;
                            casePanel = CasePanel(settings.language, id);
                            if (!running)
                                break;
                        }
                        if (event.key.scancode == SDL_SCANCODE_E)
                            inspect = true;
                        if (event.key.scancode == SDL_SCANCODE_J)
                            openJournal = true;
                        if (event.key.scancode == SDL_SCANCODE_R)
                            openReport = true;
                    }
                }
                if (!running)
                    break;

                const auto eyeBeforeMove = player.Eye();
                const auto seconds
                    = std::chrono::duration<float>(now - started).count();
                const auto boundary = apartment.rain     ? -7.65f
                                      : apartment.cinema ? -14.05f
                                                         : -12.0f;
                if (benchmarking)
                {
                    if (eyeBeforeMove.z <= boundary)
                        benchmarkDirection = -1.0f;
                    else if (eyeBeforeMove.z >= 3.9f)
                        benchmarkDirection = 1.0f;
                    yaw = (benchmarkDirection < 0.0f ? 3.14159f : 0.0f)
                          + std::sin(seconds * 1.24f) * 1.16f;
                    pitch = std::sin(seconds * 0.57f) * 0.15f;
                }
                const auto camera = View(yaw, pitch);
                const auto *keys = SDL_GetKeyboardState(nullptr);
                const float forward
                    = (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])
                      - (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]);
                const float right
                    = (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT])
                      - (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]);
                const float speed
                    = 3.05f / std::max(1.0f, std::hypot(forward, right));
                constexpr float kStep = 1.0f / 120.0f;
                while (pending >= kStep)
                {
                    if (benchmarking)
                        player.Move(0, -2.25f * benchmarkDirection, kStep);
                    else
                        player.Move(
                            (camera.right.x * right + std::sin(yaw) * forward)
                                * speed,
                            (camera.right.z * right - std::cos(yaw) * forward)
                                * speed,
                            kStep);
                    pending -= kStep;
                }

                const auto target
                    = static_cast<Subject>(player.LookAt(camera.forward));
                if (openJournal || openReport
                    || (inspect && target != Subject::kNone))
                {
                    audio.Silence();
                    auto *window = graphics.renderer->Window();
                    const auto before = investigation.Serialize();
                    SDL_SetWindowRelativeMouseMode(window, false);
                    Dialog dialog(*graphics.renderer);
                    if (openJournal)
                        casePanel.Journal(dialog, investigation);
                    else if (openReport)
                        casePanel.FileReport(dialog, investigation);
                    else
                        casePanel.Observe(dialog, investigation, target);
                    const auto after = investigation.Serialize();
                    if (before != after && !WriteLine(casePath, after))
                        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                    "Could not save case progress.");
                    if (dialog.WantsQuit())
                        break;
                    SDL_SetWindowRelativeMouseMode(window, true);
                    refreshTitle = true;
                }
                if (refreshTitle || previousTarget != target)
                {
                    const auto &text
                        = kHudText[static_cast<std::size_t>(settings.language)];
                    const auto hint = target != Subject::kNone
                                          ? casePanel.SourceName(target)
                                      : investigation.Filed() ? text.filed
                                      : investigation.IsComplete()
                                          ? text.complete
                                          : text.controls;
                    const auto title
                        = std::string(casePanel.Title()) + " | "
                          + Rendering::Name(graphics.renderer->ActiveBackend())
                                .data()
                          + " | " + hint;
                    SDL_SetWindowTitle(graphics.renderer->Window(),
                                       title.c_str());
                    previousTarget = target;
                    refreshTitle = false;
                }

                int width = 1, height = 1;
                SDL_GetWindowSizeInPixels(graphics.renderer->Window(),
                                          &width,
                                          &height);
                const auto renderSize = DesiredSize(settings);
                const auto viewport
                    = Rendering::FitViewport(renderSize, { width, height });
                if (benchmarking && now >= measureFrom
                    && (width <= 0 || height <= 0
                        || (SDL_GetWindowFlags(graphics.renderer->Window())
                            & SDL_WINDOW_MINIMIZED)
                               != 0))
                    presented = false;
                const auto eye = player.Eye();
                horror.Advance(id,
                               audio,
                               eye,
                               apartment.eventDepth,
                               std::min(elapsed, 0.06f),
                               forward * forward + right * right < 0.001f);
                audio.Update(id,
                             eye,
                             camera,
                             static_cast<float>(settings.ambienceVolume) * 0.2f,
                             static_cast<float>(settings.effectsVolume) * 0.2f);
                const auto dim
                    = std::clamp(horror.time / 1.15f, 0.0f, 1.0f)
                      * std::clamp((6.7f - horror.time) / 1.95f, 0.0f, 1.0f);
                const auto projected
                    = std::clamp((horror.time - 1.4f) * 2.0f, 0.0f, 1.0f)
                      * std::clamp((5.0f - horror.time) * 2.0f, 0.0f, 1.0f);
                BuildFrame(
                    apartment,
                    eye,
                    camera,
                    static_cast<float>(renderSize.width)
                        / static_cast<float>(renderSize.height),
                    target != Subject::kNone,
                    Quality(settings),
                    vertices,
                    static_cast<float>(
                        std::chrono::duration<double>(now - started).count()),
                    apartment.cinema ? projected : 0.0f);
                const auto caption = horror.Caption(settings.language);
                if (settings.captions && !caption.empty())
                    AppendCaption(caption,
                                  viewport.size.width,
                                  viewport.size.height,
                                  vertices);
                else if (!measuring && target != Subject::kNone)
                    AppendCaption(std::string("E  ")
                                      + casePanel.SourceName(target),
                                  viewport.size.width,
                                  viewport.size.height,
                                  vertices);
                else if (!measuring && seconds < 5.0f)
                    AppendCaption(
                        kHudText[static_cast<std::size_t>(settings.language)]
                            .controls,
                        viewport.size.width,
                        viewport.size.height,
                        vertices);
                const auto quality = Quality(settings);
                Rendering::WorldFrame worldFrame{
                    world,
                    FrameView(apartment,
                              eye,
                              camera,
                              static_cast<float>(renderSize.width)
                                  / static_cast<float>(renderSize.height),
                              quality,
                              1.0f - 0.78f * dim),
                    world->detailEnds[static_cast<std::size_t>(quality.details)]
                };
                worldFrame.view.probeCenter[3] = seconds;
                if (auto rendered = graphics.renderer->Draw(vertices,
                                                            renderSize,
                                                            &worldFrame);
                    !rendered)
                {
                    if (graphics.renderer->ActiveBackend() != Backend::kVulkan)
                    {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                                     "%s",
                                     rendered.error().c_str());
                        return EXIT_FAILURE;
                    }
                    SDL_Log("Vulkan frame failed: %s",
                            rendered.error().c_str());
                    auto fallback = Rendering::Open(Backend::kOpenGl,
                                                    DesiredSize(settings),
                                                    settings.sync);
                    if (!fallback)
                    {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                                     "%s",
                                     fallback.error().c_str());
                        return EXIT_FAILURE;
                    }
                    graphics.renderer = std::move(*fallback);
                    if (auto configured
                        = ConfigureWindow(graphics.renderer->Window(),
                                          settings);
                        !configured)
                        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                    "%s",
                                    configured.error().c_str());
                    UseWindow(graphics);
                    refreshTitle = true;
                }
                ++frames;
            }
            audio.Silence();
            if (!samples.empty())
            {
                std::ranges::sort(samples);
                const auto percentile
                    = samples.size() - samples.size() / 100 - 1;
                const auto totalMilliseconds
                    = std::accumulate(samples.begin(), samples.end(), 0.0);
                const auto averageFps
                    = totalMilliseconds > 0.0
                          ? 1000.0 * static_cast<double>(samples.size())
                                / totalMilliseconds
                          : 0.0;
                int width{}, height{};
                SDL_GetWindowSizeInPixels(graphics.renderer->Window(),
                                          &width,
                                          &height);
                SDL_Log(
                    "%s | %s render %dx%d (window %dx%d): %zu sampled frames, "
                    "%.1f FPS average, p99 %.2f ms",
                    Investigation::Definition(id).key.data(),
                    Rendering::Name(graphics.renderer->ActiveBackend()).data(),
                    benchmarkSize.width,
                    benchmarkSize.height,
                    width,
                    height,
                    samples.size(),
                    averageFps,
                    samples[percentile]);
                if (benchmarking)
                {
                    if (!presented || Timer::now() < until)
                    {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                                     "Benchmark requires a visible window and "
                                     "the full duration.");
                        return EXIT_FAILURE;
                    }
                    if (samples[percentile] > 20.0)
                        return EXIT_FAILURE;
                }
            }
            else if (benchmarking)
                return EXIT_FAILURE;
            return EXIT_SUCCESS;
        }
        auto
        Play(const Options &options, const char *executable) -> int
        {
            if (options.verifyCampaign)
            {
                bool verified = true;
                for (const auto id : kCaseIds)
                    verified = VerifyRoute(id) && verified;
                return verified ? EXIT_SUCCESS : EXIT_FAILURE;
            }
            if (options.verifyRoute)
                return VerifyRoute(options.caseId.value_or(CaseId::kApartment))
                           ? EXIT_SUCCESS
                           : EXIT_FAILURE;

            const auto path = SettingsFile();
            auto settings = ReadSettings(path);
            if (options.hasOverride)
                settings.backend = options.override;
            if (options.disableSync)
                settings.sync = false;
            if (options.resolutionOverride >= 0)
                settings.resolution = options.resolutionOverride;
            if (options.fullscreen)
                settings.fullscreen = true;
            if (options.benchmarkSeconds > 0)
            {
                settings.preset = Preset::kHigh;
                settings.fullscreen = options.fullscreen;
            }

            auto initial = Choose(settings.backend,
                                  DesiredSize(settings),
                                  settings.sync,
                                  true);
            if (!initial)
            {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                             "%s",
                             initial.error().c_str());
                return EXIT_FAILURE;
            }

            Graphics graphics{ std::move(*initial) };
            if (auto configured
                = ConfigureWindow(graphics.renderer->Window(), settings);
                !configured)
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "%s",
                            configured.error().c_str());
            UseWindow(graphics);
            if (options.verifySwitch)
            {
                const auto original = graphics.renderer->ActiveBackend();
                const auto other = original == Backend::kVulkan
                                       ? Backend::kOpenGl
                                       : Backend::kVulkan;
                auto alternative = settings;
                alternative.backend = other;
                const auto changed
                    = ApplySettings(graphics, settings, alternative, path);
                if (changed)
                {
                    if (ReadSettings(path).backend != other
                        || graphics.renderer->ActiveBackend() != other)
                        return EXIT_FAILURE;
                    alternative.backend = original;
                    if (!ApplySettings(graphics, settings, alternative, path))
                        return EXIT_FAILURE;
                    SDL_Log(
                        "Switched graphics API and verified saved setting.");
                }
                else if (graphics.renderer->ActiveBackend() != original)
                    return EXIT_FAILURE;
                else
                    SDL_Log("Unavailable graphics API left the active renderer "
                            "intact: %s",
                            changed.error().c_str());
            }

            CaseAudio audio(executable);
            auto nextCase = options.caseId;
            if (!nextCase
                && (options.frames > 0 || options.verifySwitch
                    || options.benchmarkSeconds > 0))
                nextCase = CaseId::kApartment;
            while (true)
            {
                const auto id = nextCase ? nextCase
                                         : SelectCase(graphics, settings, path);
                nextCase.reset();
                if (!id)
                    return EXIT_SUCCESS;
                bool caseList = false;
                const auto result = PlayCase(options,
                                             graphics,
                                             audio,
                                             settings,
                                             path,
                                             *id,
                                             caseList,
                                             executable);
                if (!caseList || result != EXIT_SUCCESS)
                    return result;
            }
        }
    } // namespace
} // namespace TwentyThreeSeventeen::Game

auto
main(int argc, char **argv) -> int
{
    using namespace TwentyThreeSeventeen::Game;
    if (argc == 2 && std::string_view(argv[1]) == "--version")
    {
        const auto version = BuildVersion();
        std::puts(version.empty() ? "Development build" : version.c_str());
        return EXIT_SUCCESS;
    }
    const auto options
        = ReadOptions(std::span(argv + 1, static_cast<std::size_t>(argc - 1)));
    if (!options)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s",
                     options.error().c_str());
        return EXIT_FAILURE;
    }
    if (!SDL_Init((options->verifyRoute || options->verifyCampaign)
                      ? 0
                      : SDL_INIT_VIDEO))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
        return EXIT_FAILURE;
    }
    const int result = Play(*options, argv[0]);
    SDL_Quit();
    return result;
}
