#include "CardTooltip.h"
#include "CardDelegate.h"
#include "CardDetailTooltip.h"
#include "GameState.h"
#include "ImageCache.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QScreen>
#include <QApplication>

// ── Generated-card tables ─────────────────────────────────────────────────────
// Add any card here that produces a fixed set of cards so the tooltip appears.
// Each entry has a "normal" list and a "corrupted" list (either may be empty).
QHash<QString, CardTooltip::GeneratedCards> CardTooltip::buildData()
{
    QHash<QString, GeneratedCards> d;

    // Shaladrassil (EDR_846)
    // Normal  : the 5 classic Ysera Dream cards
    // Corrupted: the 5 corrupted versions (EDR_846t1–t5), same order
    d["EDR_846"] = {
        { "DREAM_04",    // Dream            – return a minion to hand
          "DREAM_05",    // Nightmare         – +5/+5, destroyed next turn
          "DREAM_01",    // Laughing Sister   – Elusive
          "DREAM_02",    // Ysera Awakens     – 5 damage to all except Ysera
          "DREAM_03" },  // Emerald Drake     – 4-cost dragon
        { "EDR_846t2",   // Corrupted Dream           – shuffle into deck
          "EDR_846t1",   // Corrupted Nightmare       – Immune + +5/+5
          "EDR_846t3",   // Corrupted Laughing Sister – Elusive + hero Elusive
          "EDR_846t4",   // Corrupted Awakening       – 5 damage to all enemies
          "EDR_846t5" }, // Corrupted Drake
        "Normal Dream Cards",
        "Corrupted (Play a higher-cost card first)"
    };

    // Druid Nature spell pools shared by Bashana (MEND_046) and Horn of Plenty (EDR_270).
    // Standard pool sourced from hearthstone.wiki.gg/wiki/Nature (May 2026).
    // Wild-only list contains spells not legal in Standard (no duplicates).
    const QStringList druidNatureStd = {
        "CORE_EX1_169",  // Innervate             – 0
        "TLC_235",        // Life Cycle             – 1
        "CORE_AT_037",    // Living Roots           – 1
        "EDR_273",        // Symbiosis              – 1
        "TIME_701",       // Waveshaping            – 1
        "TIME_702",       // Ebb and Flow           – 2
        "CATA_138",       // Forest's Gift          – 2
        "EDR_270",        // Horn of Plenty         – 2
        "CORE_CS2_009",   // Mark of the Wild       – 2
        "CATA_135",       // Mossbinding            – 2
        "MEND_305",       // Nurturing Nature       – 2
        "EDR_843",        // Reforestation          – 2
        "CATA_561",       // Ritual of Power        – 2
        "CORE_EX1_154",   // Wrath                  – 2
        "EDR_261",        // Amphibian's Spirit     – 3
        "CORE_LOOT_373",  // Healing Rain           – 3
        "MEND_043",       // Heartroot Stones       – 3
        "EDR_848",        // Photosynthesis         – 3
        "EDR_262",        // Spirit Bond            – 3
        "CORE_LOOT_309",  // Oaken Summons          – 4
        "CATA_134",       // Wildwood Circle        – 4
        "CORE_TSC_650",   // Flipper Friends        – 5
        "TLC_236",        // Hybridization          – 5
        "EDR_233",        // Spirits of the Forest  – 5
        "TLC_230",        // TREEEES!!!             – 5
        "EDR_060",        // Ward of Earth          – 5
        "EDR_255",        // Renewing Flames        – 7
        "MEND_042",       // Lifebloom              – 9
        "EDR_232",        // Typhoon                – 10
    };
    const QStringList druidNatureWild = {
        "DRG_315",        // Embiggen               – 0
        "SCH_427",        // Lightning Bloom        – 0
        "LOOT_047",       // Barkskin               – 1
        "BOT_054",        // Biology Project        – 1
        "DAL_350",        // Crystal Power          – 1
        "BOT_444",        // Floop's Glorious Gloop – 1
        "LOOT_051",       // Lesser Jasper Spellstone – 1
        "EX1_161",        // Naturalize             – 1
        "SCH_333",        // Nature Studies         – 1
        "DRG_311",        // Treenforcements        – 1
        "GIL_663",        // Witchwood Apple        – 1
        "RLK_655",        // Wither                 – 1
        "DRG_318",        // Breath of Dreams       – 2
        "DAL_352",        // Crystalsong Portal     – 2
        "UNG_108",        // Earthen Scales         – 2
        "BT_132",         // Ironbark               – 2
        "DMF_058",        // Solar Eclipse          – 2
        "CS2_013",        // Wild Growth            – 2
        "DAL_351",        // Blessing of the Ancients – 3
        "UNG_103",        // Evolving Spores        – 3
        "BT_128",         // Fungal Fortunes        – 3
        "CS2_007",        // Healing Touch          – 3
        "CFM_713",        // Jade Blossom           – 3
        "BOT_420",        // Landscaping            – 3
        "EX1_155",        // Mark of Nature         – 3
        "AT_044",         // Mulch                  – 3
        "EX1_158",        // Soul of the Forest     – 3
        "BT_129",         // Germination            – 4
        "BOT_404",        // Juicy Psychmelon       – 4
        "TRL_254",        // Mark of the Loa        – 4
        "BT_130",         // Overgrowth             – 4
        "FP1_019",        // Poison Seeds           – 4
        "GIL_553",        // Wispering Woods        – 4
        "DRG_314",        // Aeroponics             – 5
        "YOP_026",        // Arbor Up               – 5
        "EX1_571",        // Force of Nature        – 5
        "UNG_111",        // Living Mana            – 5
        "EX1_164",        // Nourish                – 5
        "GVG_041",        // Dark Wispers           – 6
        "ULD_135",        // Hidden Oasis           – 6
        "NX2_011",        // Life from Death        – 6
        "KAR_075",        // Moonglade Portal       – 6
        "GVG_031",        // Recycle                – 6
        "SCH_612",        // Runic Carvings         – 6
        "ICC_054",        // Spreading Plague       – 6
        "ULD_273",        // Overflow               – 7
        "OG_195",         // Wisps of the Old Gods  – 7
        "DMF_732",        // Cenarion Ward          – 8
        "EX1_183",        // Gift of the Wild       – 8
        "DAL_256",        // The Forest's Aid       – 8
        "GVG_033",        // Tree of Life           – 9
    };

    // Bashana Runetotem (MEND_046) – Carve 12 Mana worth of Nature spells
    d["MEND_046"] = { druidNatureStd, druidNatureWild,
                      "Standard Nature spells", "Wild-only Nature spells", true };

    // Horn of Plenty (EDR_270) – Discover a Druid Nature spell (costs 2 less)
    d["EDR_270"]  = { druidNatureStd, druidNatureWild,
                      "Druid Nature spells (Standard)", "Wild-only Druid Nature spells", true };

    // Illidari Studies (YOP_001 / CORE_YOP_001) – Discover an Outcast card.
    // Standard pool sourced from hearthstone.wiki.gg/wiki/Outcast (May 2026).
    // Wild-only list contains Outcast cards not legal in Standard.
    const QStringList illStd = {
        "CORE_BT_480",  // Crimson Sigil Runner  – 1
        "CORE_BT_491",  // Spectral Sight         – 2
        "CORE_BT_801",  // Eye Beam               – 3
        "DINO_136",     // Horn of Feasting       – 3
        "END_005",      // Bygone Echoes          – 4
        "TIME_021",     // Doomsday Prepper       – 4
        "CATA_533",     // Flash Flood            – 5
    };
    const QStringList illWild = {
        "BT_490",       // Consume Magic          – 1
        "SCH_702",      // Felosophy              – 1
        "BT_814",       // Illidari Felblade      – 4
        "SCH_356",      // Glide                  – 4
        "SCH_705",      // Vilefiend Trainer      – 5
        "DMF_227",      // Dreadlord's Bite       – 5
        "BT_601",       // Skull of Gul'dan       – 6
        "BAR_328",      // Vengeful Spirit        – 5
        "BAR_333",      // Kurtrus Ashfallen      – 4
        "SCH_603",      // Star Student Stelina   – 5
        "SW_452",       // Chaos Leech            – 5
        "TSC_217",      // Wayward Sage           – 3
        "RLK_207",      // Fierce Outsider        – 3
        "ETC_411",      // SECURITY!!             – 2
        "GDB_116",      // Eldritch Being         – 5
        "TOY_643",      // Blind Box              – 1
        "TOY_640",      // Workshop Mishap        – 3
        "VAC_928",      // Paraglide              – 3
        "JAM_020",      // Tough Crowd            – 3
        "WW_406",       // Midnight Wolf          – 4
        "LEG_CS3_017",  // Gan'arg Glaivesmith    – 3
    };
    GeneratedCards illData = { illStd, illWild,
                               "Outcast cards (Standard)", "Wild-only Outcast cards", true };
    d["YOP_001"]      = illData;
    d["CORE_YOP_001"] = illData;

    return d;
}
const QHash<QString, CardTooltip::GeneratedCards> CardTooltip::s_data = CardTooltip::buildData();

// ── Widget ────────────────────────────────────────────────────────────────────
CardTooltip::CardTooltip(CardDatabase *cards, ImageCache *images,
                         GameState *state, QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
      m_cards(cards), m_images(images), m_state(state)
{
    setStyleSheet("QWidget { background: rgb(18,18,28); }");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(4);

    m_title = new QLabel(this);
    m_title->setStyleSheet(
        "background: rgba(20,20,30,255);"
        "color: #f0d080;"
        "padding: 6px;"
        "border-radius: 4px;"
        "font-weight: bold;"
    );
    root->addWidget(m_title);

    m_normalLbl = new QLabel(this);
    m_normalLbl->setStyleSheet("color: #aaffaa; font-weight: bold; padding: 2px;");
    root->addWidget(m_normalLbl);

    m_normalList = new QListWidget(this);
    m_normalList->setStyleSheet(
        "QListWidget { background: transparent; border: none; }"
        "QListWidget::item { padding: 0px; }"
    );
    m_normalList->setItemDelegate(new CardDelegate(cards, images, m_normalList));
    m_normalList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    root->addWidget(m_normalList);

    m_corruptedLbl = new QLabel(this);
    m_corruptedLbl->setStyleSheet("color: #ff88aa; font-weight: bold; padding: 2px;");
    root->addWidget(m_corruptedLbl);

    m_corruptedList = new QListWidget(this);
    m_corruptedList->setStyleSheet(
        "QListWidget { background: transparent; border: none; }"
        "QListWidget::item { padding: 0px; }"
    );
    m_corruptedList->setItemDelegate(new CardDelegate(cards, images, m_corruptedList));
    m_corruptedList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    root->addWidget(m_corruptedList);

    connect(images, &ImageCache::tileReady, m_normalList,    QOverload<>::of(&QWidget::update));
    connect(images, &ImageCache::tileReady, m_corruptedList, QOverload<>::of(&QWidget::update));

    // Card detail tooltip for hovering over generated cards in this panel.
    m_cardDetail = new CardDetailTooltip(cards, images);
    for (QListWidget *list : { m_normalList, m_corruptedList })
        m_cardDetail->attachToList(list, this);

    hide();
}

void CardTooltip::populateList(QListWidget *list, const QStringList &ids, bool hideCount)
{
    list->clear();
    for (const QString &id : ids) {
        auto *item = new QListWidgetItem(list);
        item->setData(Qt::UserRole,     id);
        item->setData(Qt::UserRole + 1, hideCount ? -1 : 1);
        item->setSizeHint(QSize(0, 34));
    }
    constexpr int rowH = 35;
    constexpr int maxVisible = 12;
    int naturalH = ids.size() * rowH + 2;
    int clampedH = qMin(naturalH, maxVisible * rowH);
    list->setFixedHeight(clampedH);
    list->setVerticalScrollBarPolicy(
        naturalH > clampedH ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
}

void CardTooltip::showForCard(const QString &cardId, QWidget *anchor, QRect itemGlobalRect)
{
    // ── Dynamic: Confront the Tol'vir — show 1-cost cards played this game ────
    if (m_state) {
        CardInfo ci = m_cards->lookup(cardId);
        if (ci.name == "Confront the Tol'vir") {
            const QStringList played = m_state->playerOneCostPlayed();
            m_title->setText("Confront the Tol’vir");
            m_normalLbl->setText(played.isEmpty()
                ? "No 1-cost cards played yet"
                : QString("Will replay %1 card%2:")
                      .arg(played.size()).arg(played.size() == 1 ? "" : "s"));
            m_normalLbl->setVisible(true);
            populateList(m_normalList, played);
            m_normalList->setVisible(true);
            m_corruptedLbl->setVisible(false);
            m_corruptedList->setVisible(false);

            adjustSize();
            QPoint anchorTL = anchor->mapToGlobal(QPoint(0, 0));
            int centeredY = itemGlobalRect.center().y() - height() / 2;
            QPoint pos(anchorTL.x() - width() - 4, centeredY);
            QScreen *screen = QApplication::screenAt(anchorTL);
            if (screen) {
                const QRect avail = screen->availableGeometry();
                pos.setX(qMax(avail.left(), pos.x()));
                pos.setY(qMax(avail.top(), qMin(avail.bottom() - height(), pos.y())));
            }
            move(pos);
            show();
            raise();
            return;
        }
    }

    // ── Dynamic: Animal Companion – show companions at current upgrade tier ──────
    if (cardId == "EX1_537" || cardId == "CORE_EX1_537") {
        int upgrades = m_state ? m_state->animalCompanionUpgrades() : 0;
        int cost = qMin(3 + upgrades, 10);
        m_title->setVisible(false);

        QStringList companions = m_state ? m_state->currentAnimalCompanions() : QStringList{};
        int labelCost = (companions.isEmpty() && upgrades > 0) ? cost : (upgrades == 0 ? 3 : cost);

        if (companions.isEmpty() && upgrades > 0) {
            // Upgrade played but beasts not yet detected from the log.
            m_normalLbl->setText(
                QString("Upgraded x%1 — costs %2 mana (detecting...)")
                    .arg(upgrades).arg(cost));
            m_normalLbl->setVisible(true);
            m_normalList->setVisible(false);
        } else {
            m_normalLbl->setText(
                QString("Costs %1 mana — summons one:").arg(labelCost));
            m_normalLbl->setVisible(true);
            populateList(m_normalList, companions, /*hideCount=*/true);
            m_normalList->setVisible(true);
        }
        m_corruptedLbl->setVisible(false);
        m_corruptedList->setVisible(false);

        adjustSize();
        QPoint anchorTL = anchor->mapToGlobal(QPoint(0, 0));
        int centeredY = itemGlobalRect.center().y() - height() / 2;
        QPoint pos(anchorTL.x() - width() - 4, centeredY);
        QScreen *screen = QApplication::screenAt(anchorTL);
        if (screen) {
            const QRect avail = screen->availableGeometry();
            pos.setX(qMax(avail.left(), pos.x()));
            pos.setY(qMax(avail.top(), qMin(avail.bottom() - height(), pos.y())));
        }
        move(pos);
        show();
        raise();
        return;
    }

    auto it = s_data.constFind(cardId);
    if (it == s_data.constEnd()) {
        hide();
        return;
    }

    const GeneratedCards &gc = it.value();
    CardInfo info = m_cards->lookup(cardId);
    m_title->setText(info.name.isEmpty() ? cardId : info.name);
    m_title->setVisible(true);

    m_normalLbl->setText(gc.normalLabel);
    m_normalLbl->setVisible(!gc.normal.isEmpty());
    populateList(m_normalList, gc.normal);
    m_normalList->setVisible(!gc.normal.isEmpty());

    // Wild-only section is suppressed when the player is in Standard format.
    bool showCorrupted = !gc.corrupted.isEmpty() &&
                         (!gc.corruptedIsWildOnly || !m_state || m_state->isWild());
    m_corruptedLbl->setText(gc.corruptedLabel);
    m_corruptedLbl->setVisible(showCorrupted);
    populateList(m_corruptedList, gc.corrupted);
    m_corruptedList->setVisible(showCorrupted);

    adjustSize();

    // Place to the left of the anchor panel, centered on the hovered card row.
    QPoint anchorTL = anchor->mapToGlobal(QPoint(0, 0));
    int centeredY = itemGlobalRect.center().y() - height() / 2;
    QPoint pos(anchorTL.x() - width() - 4, centeredY);

    // Clamp so it doesn't fall off screen.
    QScreen *screen = QApplication::screenAt(anchorTL);
    if (screen) {
        const QRect avail = screen->availableGeometry();
        pos.setX(qMax(avail.left(), pos.x()));
        pos.setY(qMax(avail.top(), qMin(avail.bottom() - height(), pos.y())));
    }

    move(pos);
    show();
    raise();
}
