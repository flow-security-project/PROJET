#include "VisualisationTab.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

#include "data/Salle.h"

VisualisationTab::VisualisationTab(QWidget* parent)
    : QWidget(parent)
{
    m_plot = new PlotWidget(this);

    m_anticipation = new QLabel("Anticipation : —", this);
    m_anticipation->setStyleSheet("font-size:11px;color:#555555;");
    m_conditions = new QLabel("Conditions d'évacuation : —", this);
    m_conditions->setStyleSheet("font-size:11px;color:#555555;");

    m_audio = new BarreSeuil("Audio (cris / P99)", 0, 100, true, this);
    m_therm = new BarreSeuil("Thermique (°C)", 0, 40, true, this);
    m_surface = new BarreSeuil("Surface / occupation", 0, 100, true, this);

    m_regime = new QLabel("Régime : —", this);
    m_regime->setStyleSheet("font-size:11px;color:#1A1A1A;font-weight:600;");
    m_confiance = new QLabel("Confiance : —", this);
    m_confiance->setStyleSheet("font-size:11px;color:#1A1A1A;font-weight:600;");

    m_lcd = new LcdMirror(this);

    auto* boiteCond = new QGroupBox("Conditions d'évacuation (2/3)", this);
    boiteCond->setStyleSheet(
        "QGroupBox{font-size:11px;font-weight:600;color:#1A1A1A;border:1px solid #D0D0D0;"
        "border-radius:2px;margin-top:8px;} "
        "QGroupBox::title{subcontrol-origin:margin;left:8px;top:-6px;}");
    auto* layCond = new QVBoxLayout(boiteCond);
    layCond->setContentsMargins(8, 10, 8, 8);
    layCond->addWidget(m_audio);
    layCond->addWidget(m_therm);
    layCond->addWidget(m_surface);
    layCond->addWidget(m_anticipation);

    auto* boiteInfo = new QGroupBox("État capteur", this);
    boiteInfo->setStyleSheet(
        "QGroupBox{font-size:11px;font-weight:600;color:#1A1A1A;border:1px solid #D0D0D0;"
        "border-radius:2px;margin-top:8px;} "
        "QGroupBox::title{subcontrol-origin:margin;left:8px;top:-6px;}");
    auto* layInfo = new QVBoxLayout(boiteInfo);
    layInfo->setContentsMargins(8, 10, 8, 8);
    layInfo->setSpacing(6);
    layInfo->addWidget(m_regime);
    layInfo->addWidget(m_confiance);
    layInfo->addWidget(m_lcd);
    layInfo->addStretch();

    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(8, 8, 8, 8);
    lay->setSpacing(10);
    auto* layGauche = new QVBoxLayout;
    layGauche->addWidget(m_plot, 3);
    layGauche->addWidget(m_conditions);
    lay->addLayout(layGauche, 3);
    auto* layDroite = new QVBoxLayout;
    layDroite->addWidget(boiteCond, 2);
    layDroite->addWidget(boiteInfo, 1);
    lay->addLayout(layDroite, 2);
}

void VisualisationTab::majSalle(const Salle& s)
{
    if (!s.enLigne) {
        m_plot->vider();
        m_regime->setText("Régime : hors ligne");
        m_confiance->setText("Confiance : —");
        m_anticipation->setText("Anticipation : —");
        m_conditions->setText("Conditions d'évacuation : —");
        m_audio->setValeur(0, false);
        m_therm->setValeur(0, false);
        m_surface->setValeur(0, false);
        m_lcd->afficher("HORS LIGNE", "----");
        return;
    }

    m_plot->ajouterPoint(s.taux(), s.densite);
    m_regime->setText("Régime : " + s.regime);
    m_confiance->setText(QString("Confiance : %1 %").arg(int(s.confiance * 100)));

    const int mins = qMax(0, int(s.anticipationMin));
    const int heures = mins / 60;
    m_anticipation->setText(
        heures > 0 ? QString("Anticipation saturation : %1 h %2 min")
                              .arg(heures).arg(mins % 60)
                   : QString("Anticipation saturation : %1 min").arg(mins));

    const bool condAudio = (s.rms >= 0) && (s.condAudioPct >= 50);
    const bool condTherm = (s.condThermPct >= 50);
    const bool condSurf = (s.condSurfacePct >= 50);
    int ok = 0;
    if (condAudio) ok++;
    if (condTherm) ok++;
    if (condSurf) ok++;
    m_conditions->setText(QString("Conditions d'évacuation : %1/3 %2")
                              .arg(ok)
                              .arg(ok >= 2 ? "→ déclenchement automatique" : ""));

    m_audio->setValeur(qBound(0.0, double(s.condAudioPct), 100.0), condAudio);
    m_therm->setValeur(qBound(0.0, double(s.condThermPct), 100.0), condTherm);
    m_surface->setValeur(qBound(0.0, double(s.condSurfacePct), 100.0), condSurf);

    m_lcd->afficher(s.lcdLigne1.isEmpty() ? QString("OCCUPATION %1/%2").arg(s.occupation).arg(s.capacite)
                                          : s.lcdLigne1,
                    s.lcdLigne2);
}
