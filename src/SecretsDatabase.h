#pragma once
#include <QString>
#include <QStringList>
#include <QList>

struct SecretDef {
    QString cardId;
    QString name;
    QString heroClass; // "HUNTER", "MAGE", "PALADIN", "ROGUE"
};

// Static list of known secrets. Update cardIds / add entries as new sets release.
// standardLegal is intentionally omitted — we show all class secrets always and
// let the player judge based on format. The list can be filtered by isWild() if
// needed in a future pass.
class SecretsDatabase {
public:
    static const QList<SecretDef> &all()
    {
        static const QList<SecretDef> s_all = {
            // ── Hunter ────────────────────────────────────────────────────────
            {"EX1_610",  "Explosive Trap",      "HUNTER"},
            {"EX1_611",  "Freezing Trap",       "HUNTER"},
            {"EX1_609",  "Snipe",               "HUNTER"},
            {"EX1_554",  "Snake Trap",          "HUNTER"},
            {"LOOT_079", "Wandering Monster",   "HUNTER"},
            {"GIL_577",  "Rat Trap",            "HUNTER"},
            {"KAR_004",  "Cat Trick",           "HUNTER"},
            // ── Mage ──────────────────────────────────────────────────────────
            {"EX1_287",  "Counterspell",        "MAGE"},
            {"EX1_294",  "Mirror Entity",       "MAGE"},
            {"EX1_289",  "Ice Barrier",         "MAGE"},
            {"EX1_295",  "Ice Block",           "MAGE"},
            {"EX1_283",  "Duplicate",           "MAGE"},
            {"tt_010",   "Spellbender",         "MAGE"},
            {"LOOT_101", "Explosive Runes",     "MAGE"},
            {"GIL_801",  "Splitting Image",     "MAGE"},
            {"TRL_317",  "Vaporize",            "MAGE"},
            {"KAR_076",  "Potion of Polymorph", "MAGE"},
            // ── Paladin ───────────────────────────────────────────────────────
            {"EX1_130",  "Noble Sacrifice",     "PALADIN"},
            {"EX1_132",  "Eye for an Eye",      "PALADIN"},
            {"EX1_136",  "Redemption",          "PALADIN"},
            {"EX1_379",  "Repentance",          "PALADIN"},
            {"EX1_384",  "Sacred Trial",        "PALADIN"},
            {"LOOT_088", "Hidden Wisdom",       "PALADIN"},
            {"GIL_903",  "Never Surrender!",    "PALADIN"},
            {"TRL_137",  "Autodefense Matrix",  "PALADIN"},
            // ── Rogue ─────────────────────────────────────────────────────────
            {"LOOT_214", "Cheat Death",         "ROGUE"},
            {"GIL_687",  "Evasion",             "ROGUE"},
            {"TRL_077",  "Bamboozle",           "ROGUE"},
        };
        return s_all;
    }

    // Returns cardIds of secrets for the given class.
    static QStringList cardIdsForClass(const QString &heroClass)
    {
        QStringList ids;
        for (const auto &s : all())
            if (s.heroClass == heroClass) ids.append(s.cardId);
        return ids;
    }

    // Name lookup for a cardId — fallback for cards not yet in the CardDatabase.
    static QString nameForId(const QString &cardId)
    {
        for (const auto &s : all())
            if (s.cardId == cardId) return s.name;
        return cardId;
    }
};
