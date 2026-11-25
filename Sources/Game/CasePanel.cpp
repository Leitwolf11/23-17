// SPDX-License-Identifier: MIT
#include <array>
#include <string>
#include <vector>

#include "Game/CasePanel.hpp"
#include "Game/Dialog.hpp"

namespace TwentyThreeSeventeen::Game
{
    namespace
    {
        using Investigation::Case;
        using Investigation::Claim;
        using Investigation::Subject;

        constexpr std::array kSources{
            Subject::kDispatch, Subject::kCaretaker, Subject::kCamera,
            Subject::kBreaker,  Subject::kDoorLog,   Subject::kElectrician
        };

        struct SourceText
        {
            const char *title;
            const char *body;
        };

        struct CaseText
        {
            std::array<SourceText, kSources.size()> sources;
            const char *journal;
            const char *close;
            const char *sourcePrompt;
            const char *needSource;
            const char *unrelated;
            const char *report;
            const char *supported;
            const char *incomplete;
            const char *unsupported;
            const char *emptyJournal;
            const char *readSource;
            const char *chooseSource;
        };

        constexpr std::array<CaseText, 3> kText{ {
            {
                { {
                    { "Dispatch log",
                      "23:14. The electrician radioed from the entrance after "
                      "ringing the bell. The dispatch clock is synchronized." },
                    { "Caretaker",
                      "“I recognized Alex on the fourth-floor landing at "
                      "23:17. I locked the service exit at 23:20. It stayed "
                      "shut until the police arrived.”" },
                    { "Entrance camera",
                      "Camera clock: the bell rang at 23:10. Alex entered at "
                      "23:14 on the same recording." },
                    { "Lighting fault log",
                      "The fourth-floor lights were out from 23:11 to 23:22. "
                      "The windowless landing had no working emergency "
                      "light." },
                    { "Service exit log",
                      "The synchronized door sensor recorded the service exit "
                      "opening at 23:21. The log records door movement only." },
                    { "Electrician",
                      "“I went upstairs after ringing the bell. The landing "
                      "was dark. I heard someone on the stairs but couldn't "
                      "identify them.”" },
                } },
                "Case journal",
                "Close",
                "Which collected source addresses this claim?",
                "Collect the sources that can establish this finding.",
                "That source does not settle this claim. You can try a "
                "different one.",
                "File report",
                "Report supported",
                "Report incomplete",
                "Report unsupported",
                "No sources collected yet.",
                "Read a source",
                "Choose a collected source.",
            },
            {
                { {
                    { "Leitstellenprotokoll",
                      "23:14 Uhr. Die Elektrikerin meldete sich nach dem "
                      "Klingeln per Funk vom Eingang. Die Leitstellenuhr ist "
                      "synchronisiert." },
                    { "Hausmeister",
                      "„Ich habe Alex um 23:17 Uhr im vierten Stock erkannt. "
                      "Um 23:20 Uhr habe ich den Dienstausgang abgeschlossen. "
                      "Er blieb zu, bis die Polizei kam.“" },
                    { "Eingangskamera",
                      "Kamerauhr: Um 23:10 Uhr wurde geklingelt. Laut "
                      "derselben Aufnahme kam Alex um 23:14 Uhr herein." },
                    { "Beleuchtungsprotokoll",
                      "Im vierten Stock fiel das Licht von 23:11 bis 23:22 Uhr "
                      "aus. Der fensterlose Treppenabsatz hatte kein "
                      "funktionierendes Notlicht." },
                    { "Protokoll des Dienstausgangs",
                      "Der synchronisierte Türsensor registrierte um 23:21 "
                      "Uhr, dass der Dienstausgang geöffnet wurde. Er erfasst "
                      "nur die Türbewegung." },
                    { "Elektrikerin",
                      "„Nach dem Klingeln ging ich nach oben. Der "
                      "Treppenabsatz war dunkel. Ich hörte jemanden im "
                      "Treppenhaus, konnte die Person aber nicht erkennen.“" },
                } },
                "Fallakte",
                "Schließen",
                "Welche erfasste Quelle betrifft diese Aussage?",
                "Sammle die Quellen, die diese Feststellung belegen können.",
                "Diese Quelle klärt die Aussage nicht. Versuche es mit einer "
                "anderen.",
                "Bericht einreichen",
                "Bericht belegt",
                "Bericht unvollständig",
                "Bericht unbelegt",
                "Noch keine Quellen erfasst.",
                "Quelle lesen",
                "Wähle eine erfasste Quelle.",
            },
            {
                { {
                    { "Journal d’intervention",
                      "23 h 14. L’électricienne a appelé par radio depuis "
                      "l’entrée après avoir sonné. L’horloge du poste est "
                      "synchronisée." },
                    { "Gardien",
                      "« J’ai reconnu Alex sur le palier du quatrième à 23 h "
                      "17. J’ai verrouillé la sortie de service à 23 h 20. "
                      "Elle est restée fermée jusqu’à l’arrivée de la police. "
                      "»" },
                    { "Caméra d’entrée",
                      "Horloge de la caméra : la sonnette retentit à 23 h 10. "
                      "Alex entre à 23 h 14 sur le même enregistrement." },
                    { "Relevé d’éclairage",
                      "L’éclairage du quatrième était coupé de 23 h 11 à 23 h "
                      "22. Le palier sans fenêtre n’avait aucun éclairage de "
                      "secours fonctionnel." },
                    { "Registre de la sortie",
                      "Le capteur synchronisé a enregistré l’ouverture de la "
                      "sortie de service à 23 h 21. Il relève uniquement les "
                      "mouvements de la porte." },
                    { "Électricienne",
                      "« Je suis montée après avoir sonné. Le palier était "
                      "dans le noir. J’ai entendu quelqu’un dans l’escalier "
                      "sans pouvoir l’identifier. »" },
                } },
                "Carnet d’enquête",
                "Fermer",
                "Quelle source recueillie concerne cette déclaration ?",
                "Réunis les pièces qui peuvent établir ce constat.",
                "Cette source ne permet pas de trancher. Essaie une autre "
                "source.",
                "Remettre le rapport",
                "Rapport étayé",
                "Rapport incomplet",
                "Rapport non étayé",
                "Aucune source recueillie.",
                "Lire une source",
                "Choisis une source recueillie.",
            },
        } };

        using Investigation::CaseId;
        using Localized = std::array<const char *, 3>;

        struct LocalSource
        {
            Localized title;
            Localized body;
        };

        constexpr std::array<LocalSource, 16> kNewSources{ {
            { { "Stair microphone recorder",
                "Treppenrekorder",
                "Enregistreur de l’escalier" },
              { "23:19. The full stair recording captures Alex testing the "
                "handrail, one set of steps, the bracket giving way, and the "
                "fall. No door or second walker is audible. The contact sensor "
                "confirms the railing failed before the impact.",
                "23:19. Die Aufnahme erfasst Alex am Geländer, eine einzelne "
                "Schrittfolge, das Brechen der Halterung und den Sturz. Keine "
                "Tür und keine zweite Person sind zu hören. Der Kontaktsensor "
                "bestätigt, dass das Geländer vor dem Aufprall nachgab.",
                "23:19. La piste intégrale capte Alex qui éprouve la rampe, "
                "une seule série de pas, la rupture du support et la chute. "
                "Aucune porte ni seconde personne ne s’entend. Le capteur "
                "confirme que la rampe a cédé avant le choc." } },
            { { "Maintenance revision strip",
                "Änderungsprotokoll",
                "Historique de maintenance" },
              { "The original inspection marked the lower rail unsafe. At "
                "23:34 building manager Mara Voss replaced the warning with "
                "‘repair complete’. Her signature matches the original "
                "inspection and the key register.",
                "Die Erstprüfung stufte das untere Geländer als unsicher ein. "
                "Um 23:34 ersetzte Hausverwalterin Mara Voss die Warnung durch "
                "den Vermerk „Reparatur abgeschlossen“. Ihre Unterschrift "
                "stimmt mit der Erstprüfung und dem Schlüsselbuch überein.",
                "Le contrôle initial signalait la rampe basse comme "
                "dangereuse. À 23:34, la régisseuse Mara Voss a remplacé "
                "l’alerte par la mention « réparation terminée ». Sa signature "
                "correspond au contrôle initial et au registre des clés." } },
            { { "Inventory terminal",
                "Inventarrechner",
                "Terminal d’inventaire" },
              { "Box A-41 contains flood relocation files. The 23:00 inventory "
                "still reports it in public storage. No withdrawal is "
                "registered.",
                "Kiste A-41 enthält die Umsiedlungsakten zum Hochwasser. Um "
                "23:00 steht sie laut Inventar noch im öffentlichen Magazin. "
                "Es gibt keine Austragung.",
                "La boîte A-41 contient les dossiers de relogement après la "
                "crue. À 23:00, l’inventaire la situe encore au dépôt public. "
                "Aucune sortie n’est inscrite." } },
            { { "Archivist Iris Kern",
                "Archivarin Iris Kern",
                "L’archiviste Iris Kern" },
              { "Iris says the withdrawal was routine and the box was returned "
                "to the public shelves before her shift ended.",
                "Iris sagt, die Entnahme sei Routine gewesen. Die Kiste sei "
                "vor Schichtende ins öffentliche Magazin zurückgekehrt.",
                "Iris assure que le retrait était habituel et que la boîte a "
                "regagné les rayons publics avant sa fin de service." } },
            { { "Carbon transfer slip",
                "Durchschlag des Transportscheins",
                "Double du bon de transfert" },
              { "22:12. The carbon copy names A-41, specifies lift B, and "
                "bears the director’s override. Public material normally uses "
                "lift A.",
                "22:12. Der Durchschlag nennt A-41, Aufzug B und eine "
                "Sonderfreigabe der Direktion. Öffentliches Material nimmt "
                "normalerweise Aufzug A.",
                "22:12. Le double mentionne A-41, le monte-charge B et une "
                "dérogation de la direction. Les fonds publics passent "
                "d’ordinaire par le monte-charge A." } },
            { { "Shelf weight ledger",
                "Fachwaagenprotokoll",
                "Registre de pesée du rayon" },
              { "21:56. The weight at A-41 drops by 4.8 kg. The shelf stays "
                "empty through the closing check.",
                "21:56. Das Gewicht im Fach A-41 sinkt um 4,8 kg. Bis zur "
                "Schlusskontrolle bleibt es leer.",
                "21:56. Le rayon A-41 perd 4,8 kg. Il reste vide jusqu’au "
                "contrôle de fermeture." } },
            { { "Duty guard’s statement",
                "Aussage des Wachdienstes",
                "Déclaration du gardien" },
              { "22:17. I walked the courier from lift B to the restricted "
                "conservation corridor. The seal read A-41.",
                "22:17. Ich begleitete den Kurier von Aufzug B zum gesperrten "
                "Restaurierungsgang. Auf dem Siegel stand A-41.",
                "22:17. J’ai accompagné le coursier du monte-charge B au "
                "couloir de restauration réservé. Le scellé indiquait "
                "A-41." } },
            { { "Conservation intake",
                "Restaurierungseingang",
                "Registre de restauration" },
              { "22:24. Restricted conservation received a sealed box marked "
                "A-41, 4.8 kg, under directive D-7. No return appears on the "
                "same ledger.",
                "22:24. Die gesperrte Restaurierung nahm die versiegelte Kiste "
                "A-41 mit 4,8 kg unter Anweisung D-7 an. Das Buch verzeichnet "
                "keine Rückgabe.",
                "22:24. La restauration réservée a reçu une boîte scellée A-41 "
                "de 4,8 kg sous la directive D-7. Le registre ne mentionne "
                "aucun retour." } },
            { { "Director’s directive D-7",
                "Direktionsanweisung D-7",
                "Directive D-7" },
              { "Director Eva Holm ordered A-41 moved to her restricted "
                "collection. The note directs staff to keep its public shelf "
                "location in the catalogue.",
                "Direktorin Eva Holm ordnete den Umzug von A-41 in ihre "
                "Sperrsammlung an. Laut Vermerk sollte der öffentliche "
                "Standort im Katalog stehen bleiben.",
                "La directrice Eva Holm ordonne de placer A-41 dans son fonds "
                "réservé et de conserver sa cote publique dans le "
                "catalogue." } },
            { { "Last show schedule",
                "Plan der letzten Vorstellung",
                "Programme de la dernière séance" },
              { "The final screening was supposed to end with the balcony "
                "insert. Projectionist René Vogel was due to leave by the "
                "staff door.",
                "Die letzte Vorstellung sollte mit der Balkonaufnahme enden. "
                "Filmvorführer René Vogel sollte das Haus durch den "
                "Personalausgang verlassen.",
                "La dernière séance devait se terminer par le plan du balcon. "
                "Le projectionniste René Vogel devait sortir par la porte du "
                "personnel." } },
            { { "Usher Luc Martin",
                "Platzanweiser Luc Martin",
                "L’ouvreur Luc Martin" },
              { "Luc insists the balcony sequence played last and nobody used "
                "the staff exit after curfew.",
                "Luc behauptet, die Balkonaufnahme sei zuletzt gelaufen und "
                "nach Sperrstunde habe niemand den Personalausgang benutzt.",
                "Luc affirme que le plan du balcon est passé en dernier et que "
                "personne n’a emprunté la sortie du personnel après le "
                "couvre-feu." } },
            { { "Screening copy", "Vorführkopie", "Copie de projection" },
              { "The preserve tape shows a new splice and the balcony insert "
                "before the closing title. The reference sequence puts the "
                "same images after the title.",
                "Das Sicherungsband zeigt eine neue Klebestelle und die "
                "Balkonaufnahme vor dem Schlussbild. In der Referenzfassung "
                "folgt sie auf das Schlussbild.",
                "La bande témoin montre une collure récente et le plan du "
                "balcon avant le carton final. La copie de référence le place "
                "après le carton." } },
            { { "Projector diary", "Vorführbuch", "Journal de projection" },
              { "23:09. The projector counter records the balcony insert at "
                "position 11, before the closing title at position 12. The "
                "timing matches the preserved tape.",
                "23:09. Der Zähler setzt die Balkonaufnahme auf Position 11 "
                "und das Schlussbild auf 12. Die Zeiten passen zum "
                "Sicherungsband.",
                "23:09. Le compteur place le plan du balcon en position 11 et "
                "le carton final en position 12. L’horaire correspond à la "
                "bande témoin." } },
            { { "Staff exit log",
                "Personalausgangsprotokoll",
                "Registre de la sortie du personnel" },
              { "23:14. René Vogel signed out and the interior badge reader "
                "logged his pass. The night porter countersigned the entry.",
                "23:14. René Vogel trug sich aus. Das Lesegerät innen "
                "registrierte seinen Ausweis. Der Nachtportier zeichnete "
                "gegen.",
                "23:14. René Vogel a signé sa sortie et le lecteur intérieur a "
                "enregistré son badge. Le veilleur de nuit a contresigné." } },
            { { "Reel leader", "Vorspannband", "Amorce de la bobine" },
              { "22:58. A grease-pencil mark instructs the operator to move "
                "the balcony insert before the final title. Booth supervisor "
                "Ada Weiss initialed the change.",
                "22:58. Ein Fettstiftvermerk setzt die Balkonaufnahme vor das "
                "Schlussbild. Vorführleiterin Ada Weiss zeichnete die Änderung "
                "ab.",
                "22:58. Une note au crayon gras place le plan du balcon avant "
                "le carton final. La responsable de cabine Ada Weiss a paraphé "
                "le changement." } },
            { { "Taxi receipt and dispatch",
                "Taxiquittung und Funkbuch",
                "Reçu et registre du taxi" },
              { "23:21. Driver Salim Haddad took René Vogel from the staff "
                "exit to the night train station. The driver identified René "
                "and logged payment in person.",
                "23:21. Fahrer Salim Haddad brachte René Vogel vom "
                "Personalausgang zum Nachtbahnhof. Er identifizierte René und "
                "vermerkte die persönliche Zahlung.",
                "23:21. Le chauffeur Salim Haddad a conduit René Vogel de la "
                "sortie du personnel à la gare de nuit. Il a identifié René et "
                "consigné le paiement en personne." } },
        } };

        struct Question
        {
            Localized prompt;
            std::array<Localized, 2> choices;
        };

        struct Story
        {
            Localized title;
            Localized brief;
            Localized comparison;
            std::array<Localized, 2> claims;
            std::array<Localized, 2> findings;
            std::array<Localized, 4> events;
            std::array<Question, 3> questions;
            Localized solution;
        };

        constexpr std::array<Story, 3> kStories{ {
            {
                { "The Apartment", "Das Mietshaus", "L’immeuble" },
                { "Alex fell in the building after a power outage. Establish "
                  "when Alex arrived, what caused the fall and who rewrote the "
                  "repair record. Start with the two records in the lobby.",
                  "Alex stürzte nach einem Stromausfall. Kläre die Ankunft, "
                  "die Sturzursache und die Änderung am Reparaturbuch. Beginne "
                  "mit den zwei Unterlagen im Foyer.",
                  "Alex a chuté après une panne de courant. Établis son heure "
                  "d’arrivée, la cause de la chute et l’origine de la "
                  "modification du registre. Commence par les deux pièces du "
                  "hall." },
                { "The lobby clock is four minutes slow. The 23:14 bell marks "
                  "23:10 on camera, so Alex arrived at 23:18 in real time.",
                  "Die Foyeruhr geht vier Minuten nach. Zur Klingel um 23:14 "
                  "zeigt die Kamera 23:10. Alex kam also um 23:18 an.",
                  "L’horloge du hall retarde de quatre minutes. La sonnette de "
                  "23:14 apparaît à 23:10 sur la caméra. Alex est arrivé à "
                  "23:18." },
                { { { "The 23:17 identification",
                      "Die Beobachtung um 23:17",
                      "L’identification de 23:17" },
                    { "The locked service exit",
                      "Der verriegelte Dienstausgang",
                      "La sortie de service verrouillée" } } },
                { { { "The outage covered 23:17. The caretaker can only stand "
                      "by seeing a silhouette.",
                      "Um 23:17 herrschte Stromausfall. Der Hausmeister bleibt "
                      "nur bei der Aussage, einen Schatten gesehen zu haben.",
                      "Le courant était coupé à 23:17. Le gardien maintient "
                      "seulement avoir vu une silhouette." },
                    { "The service exit opened at 23:21. The record gives no "
                      "identity.",
                      "Der Dienstausgang öffnete sich um 23:21. Das Protokoll "
                      "nennt keine Person.",
                      "La sortie de service s’est ouverte à 23:21. Le journal "
                      "n’identifie personne." } } },
                { { { "Power fails at 23:11",
                      "Stromausfall um 23:11",
                      "Panne à 23:11" },
                    { "Alex enters after the bell",
                      "Alex kommt nach dem Klingeln",
                      "Alex entre après la sonnette" },
                    { "Railing gives way at 23:19",
                      "Geländer bricht um 23:19",
                      "La rampe cède à 23:19" },
                    { "Service exit opens at 23:21",
                      "Dienstausgang öffnet um 23:21",
                      "La sortie s’ouvre à 23:21" } } },
                { {
                    { { "What caused the fall?",
                        "Was verursachte den Sturz?",
                        "Quelle est la cause de la chute ?" },
                      { { { "The unsafe railing failed while Alex was alone.",
                            "Das unsichere Geländer gab nach, als Alex allein "
                            "war.",
                            "La rampe dangereuse a cédé alors qu’Alex était "
                            "seul." },
                          { "A person identified at 23:17 pushed Alex.",
                            "Die um 23:17 erkannte Person stieß Alex.",
                            "La personne identifiée à 23:17 a poussé "
                            "Alex." } } } },
                    { { "Who changed the maintenance record?",
                        "Wer änderte das Reparaturbuch?",
                        "Qui a modifié le registre de maintenance ?" },
                      { { { "The electrician.",
                            "Der Elektriker.",
                            "L’électricien." },
                          { "Building manager Mara Voss.",
                            "Hausverwalterin Mara Voss.",
                            "La régisseuse Mara Voss." } } } },
                    { { "What does the service exit establish?",
                        "Was belegt der Dienstausgang?",
                        "Que prouve la sortie de service ?" },
                      { { { "It opened at 23:21. The user is unidentified.",
                            "Er öffnete sich um 23:21. Die Person ist "
                            "unbekannt.",
                            "Elle s’est ouverte à 23:21. Son utilisateur reste "
                            "inconnu." },
                          { "Alex left through it before the fall.",
                            "Alex ging vor dem Sturz hinaus.",
                            "Alex l’a empruntée avant sa chute." } } } },
                } },
                { "Case closed. Alex arrived at 23:18 and fell alone when the "
                  "unsafe railing failed at 23:19. Mara Voss signed the false "
                  "repair entry at 23:34.",
                  "Fall abgeschlossen. Alex kam um 23:18 und stürzte allein, "
                  "als das unsichere Geländer um 23:19 nachgab. Mara Voss "
                  "unterzeichnete den falschen Reparaturvermerk um 23:34.",
                  "Affaire classée. Alex est arrivé à 23:18 et a chuté seul "
                  "quand la rampe a cédé à 23:19. Mara Voss a signé la fausse "
                  "réparation à 23:34." },
            },
            {
                { "Night Shift", "Nachtschicht", "Service de nuit" },
                { "The public catalogue says A-41 is on its shelf. Its files "
                  "are missing. Trace the box and establish who ordered the "
                  "transfer. Start at the intake desk.",
                  "Laut Katalog steht A-41 im Fach. Die Akten fehlen. Verfolge "
                  "die Kiste und kläre, wer den Transport anordnete. Beginne "
                  "an der Annahme.",
                  "Le catalogue public situe A-41 à son rayon. Les dossiers "
                  "ont disparu. Retrace le parcours de la boîte et identifie "
                  "l’auteur de l’ordre. Commence au guichet." },
                { "The shelf lost 4.8 kg before a signed slip transferred the "
                  "matching box via lift B.",
                  "Das Fach verlor 4,8 kg, bevor die passende Kiste laut Beleg "
                  "über Aufzug B weiterging.",
                  "Le rayon a perdu 4,8 kg avant le transfert de la boîte "
                  "correspondante par le monte-charge B." },
                { { { "The transfer was routine",
                      "Der Transport war Routine",
                      "Le transfert était habituel" },
                    { "The box was returned",
                      "Die Kiste kam zurück",
                      "La boîte a été rendue" } } },
                { { { "Iris acknowledges lift B was restricted and required an "
                      "authorization.",
                      "Iris bestätigt, dass Aufzug B gesperrt und "
                      "freigabepflichtig war.",
                      "Iris confirme que l’usage du monte-charge B exigeait "
                      "une autorisation." },
                    { "Iris admits her return statement came from the "
                      "catalogue. The intake record places the box in "
                      "conservation.",
                      "Iris räumt ein, dass sie nur den Katalog kannte. Das "
                      "Eingangsbuch belegt den Verbleib in der Restaurierung.",
                      "Iris reconnaît avoir repris le catalogue. Le registre "
                      "d’arrivée situe la boîte à la restauration." } } },
                { { { "The shelf loses 4.8 kg",
                      "Das Fach verliert 4,8 kg",
                      "Le rayon perd 4,8 kg" },
                    { "Transfer through lift B is signed",
                      "Transport über Aufzug B wird quittiert",
                      "Le transfert par le monte-charge B est signé" },
                    { "Guard escorts the courier",
                      "Wache begleitet den Kurier",
                      "Le gardien accompagne le coursier" },
                    { "Conservation receives A-41",
                      "Restaurierung nimmt A-41 an",
                      "La restauration reçoit A-41" } } },
                { {
                    { { "Where did A-41 go?",
                        "Wo landete A-41?",
                        "Où A-41 est-elle arrivée ?" },
                      { { { "The public stacks, via lift A.",
                            "Ins öffentliche Magazin über Aufzug A.",
                            "Au dépôt public, par le monte-charge A." },
                          { "Restricted conservation, via lift B.",
                            "In die gesperrte Restaurierung über Aufzug B.",
                            "À la restauration réservée, par le monte-charge "
                            "B." } } } },
                    { { "Who authorized the transfer?",
                        "Wer genehmigte den Transport?",
                        "Qui a autorisé le transfert ?" },
                      { { { "Director Eva Holm under D-7.",
                            "Direktorin Eva Holm per D-7.",
                            "La directrice Eva Holm par la directive D-7." },
                          { "The guard on duty.",
                            "Der diensthabende Wachmann.",
                            "Le gardien de service." } } } },
                    { { "Why was it still listed in public storage?",
                        "Warum blieb der öffentliche Standort eingetragen?",
                        "Pourquoi restait-elle au catalogue public ?" },
                      { { { "The courier returned it.",
                            "Der Kurier brachte sie zurück.",
                            "Le coursier l’a rapportée." },
                          { "The directive preserved the old catalogue entry.",
                            "Die Anweisung ließ den alten Katalogeintrag "
                            "bestehen.",
                            "La directive maintenait l’ancienne mention au "
                            "catalogue." } } } },
                } },
                { "Case closed. Eva Holm used D-7 to divert A-41 through lift "
                  "B into restricted conservation and directed staff to keep "
                  "the public entry.",
                  "Fall abgeschlossen. Eva Holm leitete A-41 per D-7 über "
                  "Aufzug B in die gesperrte Restaurierung und ließ den "
                  "öffentlichen Eintrag bestehen.",
                  "Affaire classée. Eva Holm a détourné A-41 par le "
                  "monte-charge B vers la restauration réservée et maintenu "
                  "l’entrée publique par la directive D-7." },
            },
            {
                { "The Last Screening",
                  "Die letzte Vorstellung",
                  "La dernière séance" },
                { "René Vogel vanished after closing. Establish the actual "
                  "screening order and the projectionist’s route. Start by the "
                  "lobby desk and talk to the usher.",
                  "René Vogel verschwand nach Schluss. Kläre die tatsächliche "
                  "Filmreihenfolge und den Weg des Vorführers. Beginne im "
                  "Foyer beim Platzanweiser.",
                  "René Vogel a disparu après la fermeture. Établis l’ordre "
                  "réel de projection et le trajet du projectionniste. "
                  "Commence dans le hall auprès de l’ouvreur." },
                { "The splice and the counter agree. The balcony insert played "
                  "before the closing title.",
                  "Klebestelle und Zähler stimmen überein. Die Balkonaufnahme "
                  "lief vor dem Schlussbild.",
                  "La collure et le compteur concordent. Le plan du balcon a "
                  "précédé le carton final." },
                { { { "The balcony sequence played last",
                      "Die Balkonaufnahme lief zuletzt",
                      "Le plan du balcon est passé en dernier" },
                    { "Nobody used the staff exit",
                      "Niemand nutzte den Personalausgang",
                      "Personne n’a pris la sortie du personnel" } } },
                { { { "Luc was following the printed schedule. The booth "
                      "leader carries an instruction to change that order.",
                      "Luc hielt sich an den gedruckten Plan. Das Vorspannband "
                      "trägt eine Anweisung zur geänderten Reihenfolge.",
                      "Luc suivait le programme imprimé. L’amorce porte une "
                      "consigne de modification." },
                    { "Luc had left that corridor. The porter countersigned "
                      "René’s sign-out.",
                      "Luc hatte den Gang verlassen. Der Nachtportier "
                      "bestätigte Renés Austragung.",
                      "Luc avait quitté le couloir. Le veilleur a contresigné "
                      "la sortie de René." } } },
                { { { "Reel leader is marked",
                      "Vorspannband wird markiert",
                      "L’amorce est annotée" },
                    { "Projector logs changed sequence",
                      "Projektor erfasst geänderte Reihenfolge",
                      "Le projecteur enregistre le nouvel ordre" },
                    { "René signs out at the staff door",
                      "René trägt sich am Personalausgang aus",
                      "René signe à la sortie du personnel" },
                    { "René pays the taxi driver",
                      "René bezahlt den Taxifahrer",
                      "René paie le chauffeur" } } },
                { {
                    { { "Which version played?",
                        "Welche Fassung lief?",
                        "Quelle version a été projetée ?" },
                      { { { "Balcony insert before the closing title.",
                            "Balkonaufnahme vor dem Schlussbild.",
                            "Le plan du balcon avant le carton final." },
                          { "The printed schedule’s order.",
                            "Die Reihenfolge aus dem Programmplan.",
                            "L’ordre prévu par le programme." } } } },
                    { { "Who authorized the change?",
                        "Wer genehmigte die Änderung?",
                        "Qui a autorisé le changement ?" },
                      { { { "Usher Luc Martin.",
                            "Platzanweiser Luc Martin.",
                            "L’ouvreur Luc Martin." },
                          { "Booth supervisor Ada Weiss.",
                            "Vorführleiterin Ada Weiss.",
                            "La responsable de cabine Ada Weiss." } } } },
                    { { "What happened to René after the show?",
                        "Was geschah nach der Vorstellung mit René?",
                        "Qu’est-il arrivé à René après la séance ?" },
                      { { { "René never left the cinema.",
                            "René blieb im Kino.",
                            "René n’a jamais quitté le cinéma." },
                          { "René left alive and paid for a taxi to the "
                            "station.",
                            "René ging lebend hinaus und bezahlte ein Taxi zum "
                            "Bahnhof.",
                            "René est sorti vivant et a payé un taxi pour la "
                            "gare." } } } },
                } },
                { "Case closed. Ada Weiss reordered the reel. René left by the "
                  "staff door at 23:14 and took a taxi to the night train "
                  "station.",
                  "Fall abgeschlossen. Ada Weiss änderte die Filmreihenfolge. "
                  "René verließ das Kino um 23:14 durch den Personalausgang "
                  "und nahm ein Taxi zum Nachtbahnhof.",
                  "Affaire classée. Ada Weiss a modifié l’ordre de la bobine. "
                  "René a quitté le cinéma à 23:14 par la sortie du personnel "
                  "et pris un taxi pour la gare." },
            },
        } };
        constexpr Localized kCompare{ "Compare records",
                                      "Unterlagen vergleichen",
                                      "Comparer les pièces" };
        constexpr Localized kFirstRecord{ "Choose the first record.",
                                          "Wähle den ersten Beleg.",
                                          "Choisis la première pièce." };
        constexpr Localized kSecondRecord{ "Choose the second record.",
                                           "Wähle den zweiten Beleg.",
                                           "Choisis la seconde pièce." };
        constexpr Localized kMismatch{
            "These records do not corroborate a finding. Look for a shared "
            "event, object, or sequence.",
            "Diese Belege stützen keinen gemeinsamen Befund. Suche ein "
            "Ereignis, Objekt oder eine Reihenfolge, die in beiden vorkommt.",
            "Ces pièces ne corroborent aucun constat. Cherche un événement, un "
            "objet ou une séquence en commun."
        };
        constexpr Localized kTimeline{ "Order events",
                                       "Ereignisse ordnen",
                                       "Ordonner les événements" };
        constexpr Localized kUnknown{ "Undiscovered event",
                                      "Unbekanntes Ereignis",
                                      "Événement inconnu" };
        constexpr Localized kEarlier{ "Move earlier",
                                      "Nach vorn verschieben",
                                      "Déplacer plus tôt" };
        constexpr Localized kLater{ "Move later",
                                    "Nach hinten verschieben",
                                    "Déplacer plus tard" };
        constexpr Localized kChallenge{ "Challenge a statement",
                                        "Aussage prüfen",
                                        "Contester une déclaration" };
        constexpr Localized kEdit{ "Draft answer",
                                   "Antwortentwurf",
                                   "Réponse provisoire" };
        constexpr Localized kSubmit{ "Submit report",
                                     "Bericht einreichen",
                                     "Remettre le rapport" };
        constexpr Localized kEvidenceNeeded{
            "Collect each source, compare the matching records, challenge both "
            "statements and complete all answers.",
            "Sammle alle Quellen, vergleiche die passenden Unterlagen, prüfe "
            "beide Aussagen und beantworte alle Fragen.",
            "Réunis les sources, compare les pièces correspondantes, conteste "
            "les deux déclarations et termine toutes les réponses."
        };
        constexpr Localized kReview{
            "Review the timeline and answers against the original records. "
            "Your draft is saved.",
            "Prüfe Reihenfolge und Antworten anhand der Originalunterlagen. "
            "Dein Entwurf wurde gespeichert.",
            "Vérifie la chronologie et les réponses à partir des pièces "
            "originales. Ton brouillon est sauvegardé."
        };

        auto
        StoryFor(CaseId id) noexcept -> const Story &
        {
            return kStories[static_cast<std::size_t>(id)];
        }

        auto
        SourceFor(Subject subject, std::size_t language) noexcept -> SourceText
        {
            const auto index = static_cast<std::size_t>(subject);
            if (index == 0 || index > kSources.size() + kNewSources.size())
                return { "", "" };
            if (index <= kSources.size())
                return kText[language].sources[index - 1];
            const auto &source = kNewSources[index - kSources.size() - 1];
            return { source.title[language], source.body[language] };
        }

        auto
        EditTimeline(Dialog &dialog, Case &investigation, std::size_t language)
            -> void
        {
            const auto &definition
                = Investigation::Definition(investigation.Id());
            const auto &story = StoryFor(investigation.Id());
            const auto &text = kText[language];
            while (!dialog.WantsQuit())
            {
                std::array<const char *, 4> events{};
                for (std::size_t pos = 0; pos < events.size(); ++pos)
                {
                    const auto event = investigation.Timeline()[pos];
                    events[pos]
                        = investigation.HasObserved(definition.timeline[event])
                              ? story.events[event][language]
                              : kUnknown[language];
                }
                const auto chosen = dialog.Choose(kTimeline[language],
                                                  "",
                                                  text.close,
                                                  events);
                if (chosen < 0)
                    return;
                const auto position = static_cast<std::size_t>(chosen);
                const auto id = investigation.Timeline()[position];
                if (!investigation.HasObserved(definition.timeline[id]))
                    continue;
                const std::array moves{ kEarlier[language], kLater[language] };
                const auto move
                    = dialog.Choose(events[position], "", text.close, moves);
                if (move >= 0)
                    investigation.MoveEvent(position, move == 0 ? -1 : 1);
            }
        }

        auto
        ChallengeClaim(Dialog &dialog,
                       Case &investigation,
                       std::size_t language) -> void
        {
            const auto &definition
                = Investigation::Definition(investigation.Id());
            const auto &story = StoryFor(investigation.Id());
            const auto &text = kText[language];
            const auto witness
                = SourceFor(definition.claims.front().witness, language);
            if (!investigation.HasObserved(definition.claims.front().witness))
            {
                dialog.Choose(kChallenge[language],
                              text.needSource,
                              text.close,
                              {});
                return;
            }
            const std::array claims{ story.claims[0][language],
                                     story.claims[1][language] };
            const auto chosen = dialog.Choose(kChallenge[language],
                                              witness.body,
                                              text.close,
                                              claims);
            if (chosen < 0)
                return;
            const auto claim = static_cast<std::size_t>(chosen);
            std::vector<const char *> names;
            std::vector<Subject> records;
            for (const auto subject : definition.sources)
            {
                if (subject == definition.claims[claim].witness
                    || !investigation.HasObserved(subject))
                    continue;
                records.push_back(subject);
                names.push_back(SourceFor(subject, language).title);
            }
            const auto evidence = dialog.Choose(claims[claim],
                                                text.sourcePrompt,
                                                text.close,
                                                names);
            if (evidence < 0)
                return;
            const auto result = investigation.Challenge(
                static_cast<Claim>(claim),
                records[static_cast<std::size_t>(evidence)]);
            dialog.Choose(claims[claim],
                          result == Investigation::ChallengeResult::kClarified
                              ? story.findings[claim][language]
                              : text.unrelated,
                          text.close,
                          {});
        }
    } // namespace

    CasePanel::CasePanel(Language language, Investigation::CaseId id) noexcept
        : language_(language)
        , id_(id)
    {}

    auto
    CasePanel::Title() const noexcept -> const char *
    {
        return StoryFor(id_).title[static_cast<std::size_t>(language_)];
    }

    auto
    CasePanel::Brief() const noexcept -> const char *
    {
        return StoryFor(id_).brief[static_cast<std::size_t>(language_)];
    }

    auto
    CasePanel::SourceName(Subject subject) const noexcept -> const char *
    {
        return SourceFor(subject, static_cast<std::size_t>(language_)).title;
    }

    auto
    CasePanel::Observe(Dialog &dialog,
                       Case &investigation,
                       Subject source) const -> void
    {
        const auto language = static_cast<std::size_t>(language_);
        investigation.Observe(source);
        const auto text = SourceFor(source, language);
        dialog.Choose(text.title, text.body, kText[language].close, {});
    }

    auto
    CasePanel::Journal(Dialog &dialog, Case &investigation) const -> void
    {
        const auto language = static_cast<std::size_t>(language_);
        const auto &text = kText[language];
        const auto &story = StoryFor(id_);
        const auto &definition = Investigation::Definition(id_);
        const std::array actions{ text.readSource,
                                  kCompare[language],
                                  kChallenge[language],
                                  kTimeline[language] };
        while (!dialog.WantsQuit())
        {
            std::string entries(Brief());
            entries += "\n";
            std::vector<const char *> names;
            std::vector<Subject> records;
            for (const auto source : definition.sources)
            {
                if (!investigation.HasObserved(source))
                    continue;
                const auto *name = SourceName(source);
                entries += std::string("\n") + name;
                records.push_back(source);
                names.push_back(name);
            }
            if (records.empty())
                entries += std::string("\n") + text.emptyJournal;
            if (investigation.HasComparedRecords())
                entries += std::string("\n\n") + story.comparison[language];
            for (std::size_t claim = 0; claim < definition.claims.size();
                 ++claim)
            {
                if (investigation.IsClarified(static_cast<Claim>(claim)))
                    entries += std::string("\n\n")
                               + story.findings[claim][language];
            }
            const auto choice
                = dialog.Choose(text.journal, entries, text.close, actions);
            if (choice == 0)
            {
                const auto selected = dialog.Choose(text.readSource,
                                                    text.chooseSource,
                                                    text.close,
                                                    names);
                if (selected >= 0)
                    Observe(dialog,
                            investigation,
                            records[static_cast<std::size_t>(selected)]);
            }
            else if (choice == 1)
            {
                if (records.size() < 2)
                {
                    dialog.Choose(kCompare[language],
                                  text.needSource,
                                  text.close,
                                  {});
                    continue;
                }
                const auto first = dialog.Choose(kCompare[language],
                                                 kFirstRecord[language],
                                                 text.close,
                                                 names);
                if (first < 0)
                    continue;
                const auto index = static_cast<std::size_t>(first);
                const auto firstSource = records[index];
                records.erase(records.begin() + first);
                names.erase(names.begin() + first);
                const auto second = dialog.Choose(kCompare[language],
                                                  kSecondRecord[language],
                                                  text.close,
                                                  names);
                if (second < 0)
                    continue;
                const auto compared = investigation.CompareRecords(
                    firstSource,
                    records[static_cast<std::size_t>(second)]);
                dialog.Choose(kCompare[language],
                              compared ? story.comparison[language]
                                       : kMismatch[language],
                              text.close,
                              {});
            }
            else if (choice == 2)
                ChallengeClaim(dialog, investigation, language);
            else if (choice == 3)
                EditTimeline(dialog, investigation, language);
            else
                return;
        }
    }

    auto
    CasePanel::FileReport(Dialog &dialog, Case &investigation) const -> void
    {
        const auto language = static_cast<std::size_t>(language_);
        const auto &text = kText[language];
        const auto &story = StoryFor(id_);
        std::array<const char *, 5> actions{};
        for (std::size_t index = 0; index < story.questions.size(); ++index)
            actions[index] = story.questions[index].prompt[language];
        actions[3] = kTimeline[language];
        actions[4] = kSubmit[language];
        while (!dialog.WantsQuit())
        {
            std::string draft(investigation.Filed() ? story.solution[language]
                                                    : "");
            for (std::size_t index = 0; index < story.questions.size(); ++index)
            {
                if (const auto answer = investigation.Answer(index);
                    answer != 0)
                    draft += std::string("\n")
                             + story.questions[index]
                                   .choices[answer - 1][language];
            }
            const auto choice
                = dialog.Choose(text.report, draft, text.close, actions);
            if (choice < 0)
                return;
            if (choice == 4)
            {
                switch (investigation.Submit())
                {
                    case Investigation::ReportFinding::kIncomplete:
                        dialog.Choose(text.incomplete,
                                      kEvidenceNeeded[language],
                                      text.close,
                                      {});
                        break;
                    case Investigation::ReportFinding::kUnsupported:
                        dialog.Choose(text.unsupported,
                                      kReview[language],
                                      text.close,
                                      {});
                        break;
                    case Investigation::ReportFinding::kSupported:
                        dialog.Choose(text.supported,
                                      story.solution[language],
                                      text.close,
                                      {});
                        return;
                }
            }
            else if (choice == 3)
                EditTimeline(dialog, investigation, language);
            else
            {
                const auto questionIndex = static_cast<std::size_t>(choice);
                const auto &question = story.questions[questionIndex];
                const std::array choices{ question.choices[0][language],
                                          question.choices[1][language] };
                const auto answer = dialog.Choose(question.prompt[language],
                                                  kEdit[language],
                                                  text.close,
                                                  choices);
                if (answer >= 0)
                    investigation.SetAnswer(
                        questionIndex,
                        static_cast<std::uint8_t>(answer + 1));
            }
        }
    }
} // namespace TwentyThreeSeventeen::Game
