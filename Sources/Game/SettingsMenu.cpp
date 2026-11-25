// SPDX-License-Identifier: MIT
#include <array>

#include "Game/Dialog.hpp"
#include "Game/SettingsMenu.hpp"

namespace TwentyThreeSeventeen::Game
{
    namespace
    {
        struct Text
        {
            const char *paused;
            const char *begin;
            const char *resume;
            const char *quit;
            const char *settings;
            const char *display;
            const char *graphics;
            const char *controls;
            const char *language;
            const char *apply;
            const char *discard;
            const char *back;
            const char *resolution;
            const char *windowMode;
            const char *windowed;
            const char *fullscreen;
            const char *currentWindow;
            const char *quality;
            const char *details;
            const char *lighting;
            const char *automatic;
            const char *activeBackend;
            const char *yes;
            const char *no;
            const char *sensitivity;
            const char *invert;
            const char *settingsError;
            const char *help;
        };

        constexpr std::array<Text, 3> kText{ {
            { "Paused",
              "Begin investigation",
              "Resume investigation",
              "Quit to desktop",
              "Settings",
              "Display",
              "Graphics",
              "Controls",
              "Language",
              "Apply changes",
              "Discard changes",
              "Back",
              "Render resolution",
              "Window mode",
              "Windowed",
              "Borderless fullscreen",
              "Current window",
              "Quality preset",
              "Scene detail",
              "Lighting detail",
              "Automatic",
              "Active API",
              "On",
              "Off",
              "Mouse sensitivity",
              "Invert vertical look",
              "Settings could not be applied",
              "WASD  Move     E  Inspect     J  Journal     R  Report" },
            { "Pause",
              "Ermittlung beginnen",
              "Ermittlung fortsetzen",
              "Spiel beenden",
              "Einstellungen",
              "Anzeige",
              "Grafik",
              "Steuerung",
              "Sprache",
              "Änderungen übernehmen",
              "Änderungen verwerfen",
              "Zurück",
              "Renderauflösung",
              "Fenstermodus",
              "Fenster",
              "Randloser Vollbildmodus",
              "Aktuelles Fenster",
              "Grafikprofil",
              "Szenendetails",
              "Beleuchtung",
              "Automatisch",
              "Aktive API",
              "An",
              "Aus",
              "Mausempfindlichkeit",
              "Vertikale Sicht umkehren",
              "Einstellungen konnten nicht angewendet werden",
              "WASD  Gehen     E  Untersuchen     J  Akte     R  Bericht" },
            { "Pause",
              "Commencer l’enquête",
              "Reprendre l’enquête",
              "Quitter le jeu",
              "Paramètres",
              "Affichage",
              "Graphismes",
              "Commandes",
              "Langue",
              "Appliquer",
              "Ignorer les modifications",
              "Retour",
              "Résolution de rendu",
              "Mode d’affichage",
              "Fenêtré",
              "Plein écran sans bordure",
              "Fenêtre actuelle",
              "Préréglage graphique",
              "Détails de la scène",
              "Éclairage",
              "Automatique",
              "API active",
              "Activé",
              "Désactivé",
              "Sensibilité de la souris",
              "Inverser la visée verticale",
              "Impossible d’appliquer les paramètres",
              "WASD  Déplacement     E  Examiner     J  Carnet     R  "
              "Rapport" },
        } };

        constexpr std::array kQuality{ "Low",   "Medium", "High",
                                       "Xhigh", "Max",    "Custom" };
        constexpr std::array kSensitivity{ "50%",
                                           "75%",
                                           "100%",
                                           "125%",
                                           "150%" };
        constexpr std::array kLanguages{ "English (US)",
                                         "Deutsch",
                                         "Français" };
        constexpr std::array kBackends{ Rendering::Backend::kAuto,
                                        Rendering::Backend::kVulkan,
                                        Rendering::Backend::kOpenGl };

        auto
        For(Language language) noexcept -> const Text &
        {
            return kText[static_cast<std::size_t>(language)];
        }

        auto
        Value(std::string_view label, std::string_view value) -> std::string
        {
            return std::string(label) + "   ·   " + std::string(value);
        }

        auto
        Resolution(Rendering::WindowSize size) -> std::string
        {
            return std::to_string(size.width) + " × "
                   + std::to_string(size.height);
        }

        template<std::size_t Count>
        auto
        Rows(Dialog &dialog,
             const char *title,
             std::string_view body,
             const char *close,
             const std::array<std::string, Count> &labels) -> int
        {
            std::array<const char *, Count> choices{};
            for (std::size_t index = 0; index < Count; ++index)
                choices[index] = labels[index].c_str();
            return dialog.Choose(title, body, close, choices);
        }

        template<std::size_t Count>
        auto
        Pick(Dialog &dialog,
             const char *title,
             const char *back,
             const std::array<const char *, Count> &options,
             int current) -> int
        {
            const auto chosen = dialog.Choose(
                title,
                Value(title, options[static_cast<std::size_t>(current)]),
                back,
                options);
            return chosen < 0 ? current : chosen;
        }

        auto
        Display(Dialog &dialog,
                Settings &settings,
                const Text &text,
                Rendering::WindowSize actual) -> void
        {
            const std::array modes{ text.windowed, text.fullscreen };
            const std::array toggles{ text.no, text.yes };
            while (!dialog.WantsQuit())
            {
                const std::array rows{
                    Value(text.resolution,
                          Resolution(kResolutions[static_cast<std::size_t>(
                              settings.resolution)])),
                    Value(text.windowMode,
                          modes[static_cast<std::size_t>(settings.fullscreen)]),
                    Value("VSync", settings.sync ? text.yes : text.no),
                };
                switch (Rows(dialog,
                             text.display,
                             Value(text.currentWindow, Resolution(actual)),
                             text.back,
                             rows))
                {
                    case 0:
                    {
                        std::array<std::string, kResolutions.size()> labels;
                        for (std::size_t index = 0; index < labels.size();
                             ++index)
                            labels[index] = Resolution(kResolutions[index]);
                        const auto current
                            = static_cast<std::size_t>(settings.resolution);
                        const auto selected
                            = Rows(dialog,
                                   text.resolution,
                                   Value(text.resolution, labels[current]),
                                   text.back,
                                   labels);
                        if (selected >= 0)
                            settings.resolution = selected;
                        break;
                    }
                    case 1:
                        settings.fullscreen
                            = Pick(dialog,
                                   text.windowMode,
                                   text.back,
                                   modes,
                                   static_cast<int>(settings.fullscreen))
                              != 0;
                        break;
                    case 2:
                        settings.sync = Pick(dialog,
                                             "VSync",
                                             text.back,
                                             toggles,
                                             static_cast<int>(settings.sync))
                                        != 0;
                        break;
                    default:
                        return;
                }
            }
        }

        auto
        Graphics(Dialog &dialog,
                 Settings &settings,
                 const Text &text,
                 Rendering::Backend active) -> void
        {
            const std::array apis{ text.automatic, "Vulkan", "OpenGL 4.2" };
            constexpr std::array levels{ "Low",
                                         "Medium",
                                         "High",
                                         "Xhigh",
                                         "Max" };
            while (!dialog.WantsQuit())
            {
                const auto apiIndex
                    = static_cast<std::size_t>(settings.backend);
                const auto presetIndex
                    = static_cast<std::size_t>(settings.preset);
                const auto quality = Quality(settings);
                const std::array rows{
                    Value("API", apis[apiIndex]),
                    Value(text.quality, kQuality[presetIndex]),
                    Value(text.details,
                          levels[static_cast<std::size_t>(quality.details)]),
                    Value(text.lighting,
                          levels[static_cast<std::size_t>(quality.lighting)]),
                };
                const auto selected
                    = Rows(dialog,
                           text.graphics,
                           Value(text.activeBackend, Rendering::Name(active)),
                           text.back,
                           rows);
                switch (selected)
                {
                    case 0:
                        settings.backend = kBackends[static_cast<std::size_t>(
                            Pick(dialog,
                                 "API",
                                 text.back,
                                 apis,
                                 static_cast<int>(settings.backend)))];
                        break;
                    case 1:
                    {
                        const auto previous = Quality(settings);
                        settings.preset = static_cast<Preset>(
                            Pick(dialog,
                                 text.quality,
                                 text.back,
                                 kQuality,
                                 static_cast<int>(settings.preset)));
                        if (settings.preset == Preset::kCustom
                            && presetIndex
                                   != static_cast<std::size_t>(Preset::kCustom))
                            settings.custom = previous;
                        break;
                    }
                    case 2:
                    case 3:
                    {
                        const bool isScene = selected == 2;
                        const auto current
                            = isScene ? quality.details : quality.lighting;
                        const auto level
                            = Pick(dialog,
                                   isScene ? text.details : text.lighting,
                                   text.back,
                                   levels,
                                   current);
                        if (level != current)
                        {
                            settings.preset = Preset::kCustom;
                            settings.custom = quality;
                            if (isScene)
                                settings.custom.details = level;
                            else
                                settings.custom.lighting = level;
                        }
                        break;
                    }
                    default:
                        return;
                }
            }
        }

        auto
        Controls(Dialog &dialog, Settings &settings, const Text &text) -> void
        {
            const std::array toggles{ text.no, text.yes };
            while (!dialog.WantsQuit())
            {
                const std::array rows{
                    Value(text.sensitivity,
                          kSensitivity[static_cast<std::size_t>(
                              settings.sensitivity)]),
                    Value(text.invert,
                          settings.invertMouse ? text.yes : text.no),
                };
                switch (Rows(dialog, text.controls, "", text.back, rows))
                {
                    case 0:
                        settings.sensitivity = Pick(dialog,
                                                    text.sensitivity,
                                                    text.back,
                                                    kSensitivity,
                                                    settings.sensitivity);
                        break;
                    case 1:
                        settings.invertMouse
                            = Pick(dialog,
                                   text.invert,
                                   text.back,
                                   toggles,
                                   static_cast<int>(settings.invertMouse))
                              != 0;
                        break;
                    default:
                        return;
                }
            }
        }
        constexpr std::array kAudio{ "Audio", "Ton", "Audio" };

        auto
        EditAudio(Dialog &dialog, Settings &settings, const Text &text) -> void
        {
            constexpr std::array ambience{ "Ambience and score",
                                           "Umgebung und Musik",
                                           "Ambiance et musique" };
            constexpr std::array effects{ "Sudden effects",
                                          "Plötzliche Effekte",
                                          "Effets soudains" };
            constexpr std::array captions{ "Sound captions",
                                           "Geräuschuntertitel",
                                           "Sous-titres des sons" };
            constexpr std::array volume{ "0%",  "20%", "40%",
                                         "60%", "80%", "100%" };
            const auto language = static_cast<std::size_t>(settings.language);
            while (!dialog.WantsQuit())
            {
                const std::array rows{
                    Value(ambience[language],
                          volume[static_cast<std::size_t>(
                              settings.ambienceVolume)]),
                    Value(effects[language],
                          volume[static_cast<std::size_t>(
                              settings.effectsVolume)]),
                    Value(captions[language],
                          settings.captions ? text.yes : text.no),
                };
                switch (Rows(dialog, kAudio[language], "", text.back, rows))
                {
                    case 0:
                        settings.ambienceVolume = Pick(dialog,
                                                       ambience[language],
                                                       text.back,
                                                       volume,
                                                       settings.ambienceVolume);
                        break;
                    case 1:
                        settings.effectsVolume = Pick(dialog,
                                                      effects[language],
                                                      text.back,
                                                      volume,
                                                      settings.effectsVolume);
                        break;
                    case 2:
                        settings.captions
                            = Pick(dialog,
                                   captions[language],
                                   text.back,
                                   std::array{ text.no, text.yes },
                                   static_cast<int>(settings.captions))
                              != 0;
                        break;
                    default:
                        return;
                }
            }
        }

    } // namespace

    auto
    StartMenu(Dialog &dialog, Language language) -> MenuAction
    {
        constexpr std::array labels{ "Return to case files",
                                     "Zurück zu den Fallakten",
                                     "Retour aux dossiers" };
        const auto &text = For(language);
        const std::array choices{ text.resume,
                                  text.settings,
                                  labels[static_cast<std::size_t>(language)],
                                  text.quit };
        const auto selected
            = dialog.Choose(text.paused, text.help, text.resume, choices);
        if (dialog.WantsQuit() || selected == 3)
            return MenuAction::kQuit;
        if (selected == 2)
            return MenuAction::kCaseList;
        return selected == 1 ? MenuAction::kSettings : MenuAction::kContinue;
    }

    auto
    EditSettings(Dialog &dialog,
                 Settings &settings,
                 Rendering::Backend active,
                 Rendering::WindowSize actualSize) -> bool
    {
        while (!dialog.WantsQuit())
        {
            const auto &text = For(settings.language);
            const std::array rows{
                std::string(text.display),
                std::string(text.graphics),
                std::string(text.controls),
                Value(text.language,
                      kLanguages[static_cast<std::size_t>(settings.language)]),
                std::string(
                    kAudio[static_cast<std::size_t>(settings.language)]),
                std::string(text.apply)
            };
            switch (Rows(dialog, text.settings, "", text.discard, rows))
            {
                case 0:
                    Display(dialog, settings, text, actualSize);
                    break;
                case 1:
                    Graphics(dialog, settings, text, active);
                    break;
                case 2:
                    Controls(dialog, settings, text);
                    break;
                case 3:
                    settings.language = static_cast<Language>(
                        Pick(dialog,
                             text.language,
                             text.back,
                             kLanguages,
                             static_cast<int>(settings.language)));
                    break;
                case 4:
                    EditAudio(dialog, settings, text);
                    break;
                case 5:
                    return true;
                default:
                    return false;
            }
        }
        return false;
    }

    auto
    ShowSettingsError(Dialog &dialog, Language language, std::string_view error)
        -> void
    {
        const auto &text = For(language);
        dialog.Choose(text.settingsError, error, text.back, {});
    }
} // namespace TwentyThreeSeventeen::Game
