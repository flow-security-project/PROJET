#include "AlertePanelWidget.h"

#include <QCheckBox>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "models/Alerte.h"
#include "models/AlerteModel.h"

static QString severiteHex(const QString& severite)
{
    if (severite == QLatin1String("critique"))
        return QStringLiteral("#C62828");
    if (severite == QLatin1String("attention"))
        return QStringLiteral("#F57C00");
    return QStringLiteral("#1E88E5");
}

AlertePanelWidget::AlertePanelWidget(AlerteModel* modele, QWidget* parent)
    : QWidget(parent)
    , m_modele(modele)
{
    setObjectName(QStringLiteral("alertePanel"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto* titre = new QLabel(QStringLiteral("ALERTES TEMPS RÉEL (Toutes Salles)"), this);
    titre->setObjectName(QStringLiteral("alertePanelTitle"));

    m_filtreNonAcq = new QCheckBox(QStringLiteral("Non acquittées uniquement"), this);
    m_filtreNonAcq->setObjectName(QStringLiteral("filtreAlertes"));

    auto* zone = new QScrollArea(this);
    zone->setObjectName(QStringLiteral("alerteScroll"));
    zone->setWidgetResizable(true);
    zone->setFrameShape(QFrame::NoFrame);
    m_liste = new QWidget(zone);
    m_liste->setObjectName(QStringLiteral("alerteListe"));
    m_layListe = new QVBoxLayout(m_liste);
    m_layListe->setContentsMargins(0, 0, 0, 0);
    m_layListe->setSpacing(4);
    m_layListe->addStretch();
    zone->setWidget(m_liste);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 10);
    layout->setSpacing(6);
    layout->addWidget(titre);
    layout->addWidget(m_filtreNonAcq);
    layout->addWidget(zone, 1);

    if (m_modele) {
        connect(m_modele, &AlerteModel::alerteAjoutee,
                this, [this](const Alerte&) { reconstruire(); });
        connect(m_modele, &AlerteModel::alerteModifiee,
                this, [this](const Alerte&) { reconstruire(); });
        connect(m_modele, &AlerteModel::alerteVidee,
                this, [this]() { reconstruire(); });
    }
    connect(m_filtreNonAcq, &QCheckBox::toggled,
            this, [this](bool) { reconstruire(); });

    reconstruire();
}

int AlertePanelWidget::nbAlertes() const
{
    return m_modele ? m_modele->alertes().size() : 0;
}

void AlertePanelWidget::reconstruire()
{
    if (!m_layListe)
        return;
    while (QLayoutItem* item = m_layListe->takeAt(0)) {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }
    m_layListe->addStretch();

    if (!m_modele)
        return;

    QList<Alerte> liste = m_modele->alertes();
    std::sort(liste.begin(), liste.end(),
              [](const Alerte& gauche, const Alerte& droite) {
                  return gauche.ts > droite.ts;
              });
    for (const Alerte& a : liste) {
        if (m_filtreNonAcq->isChecked() && a.acquittee)
            continue;
        ajouterRangee(a);
    }
}

void AlertePanelWidget::ajouterRangee(const Alerte& a)
{
    auto* rangee = new QFrame(m_liste);
    rangee->setObjectName(QStringLiteral("carteAlerte"));
    rangee->setProperty("severite", a.severite());
    rangee->setProperty("acquitte", a.acquittee);
    rangee->setProperty("salleId", a.salleId);
    rangee->setProperty("ts", a.ts);

    auto* badge = new QLabel(a.typeLibelle(), rangee);
    badge->setObjectName(QStringLiteral("alerteBadge"));
    badge->setProperty("severite", a.severite());

    auto* detail = new QLabel(rangee);
    detail->setObjectName(QStringLiteral("alerteDetail"));
    detail->setWordWrap(true);
    detail->setTextFormat(Qt::RichText);
    detail->setText(
        QStringLiteral("<b>%1</b> · <span style=\"font-family:monospace;"
                       "color:#555555;\">%2</span><br>%3<br>"
                       "<span style=\"color:#888888;\">%4 · Appel : %5</span>")
            .arg(a.salleNom.isEmpty() ? a.salleId : a.salleNom)
            .arg(QDateTime::fromMSecsSinceEpoch(a.ts).toString(QStringLiteral("hh:mm:ss")))
            .arg(a.detail)
            .arg(a.acquittee ? QStringLiteral("ACQUITTÉE")
                             : QStringLiteral("non acquittée"))
            .arg(a.appelStatutTexte()));

    auto* btnOk = new QPushButton(QStringLiteral("✓"), rangee);
    btnOk->setObjectName(QStringLiteral("btnAcquitter"));
    btnOk->setToolTip(QStringLiteral("Acquitter l'alerte"));
    btnOk->setFixedSize(26, 26);
    auto* btnVoir = new QPushButton(QStringLiteral("Voir"), rangee);
    btnVoir->setObjectName(QStringLiteral("btnVoirAlerte"));
    btnVoir->setToolTip(QStringLiteral("Ouvrir le détail de la salle"));
    btnVoir->setFixedHeight(26);

    auto* layBtns = new QVBoxLayout;
    layBtns->setSpacing(4);
    layBtns->addWidget(btnOk);
    layBtns->addWidget(btnVoir);
    layBtns->addStretch();

    auto* layout = new QHBoxLayout(rangee);
    layout->setContentsMargins(10, 6, 8, 6);
    layout->setSpacing(10);
    layout->addWidget(badge, 0, Qt::AlignTop);
    layout->addWidget(detail, 1);
    layout->addLayout(layBtns);

    m_layListe->insertWidget(m_layListe->count() - 1, rangee);

    connect(btnOk, &QPushButton::clicked, this, [this, rangee]() {
        if (m_modele)
            m_modele->acquitter(rangee->property("ts").toULongLong());
    });
    connect(btnVoir, &QPushButton::clicked, this, [this, rangee]() {
        emit voirDetailAlerte(rangee->property("salleId").toString(),
                              rangee->property("ts").toULongLong());
    });
}
