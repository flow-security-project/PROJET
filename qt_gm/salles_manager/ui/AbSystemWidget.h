#pragma once

#include <QTimer>
#include <QWidget>

// Visualisation du système A-B (vue de dessus de la porte) :
//   - Capteur A (VL53L0X / ToF) à l'extérieur, devant la porte.
//   - Capteur B (HC-SR04 / ultrason) à l'intérieur, derrière la porte.
// États affichés : attente, A vu (entrée en cours), B vu (sortie en cours),
// plus le dernier passage validé.
// Chaque passage validé déclenche une courte animation A->B (entrée) ou
// B->A (sortie) pour visualiser la séquence ; un état réel arrivant pendant
// l'animation (mode MQTT) la remplace immédiatement.
class AbSystemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AbSystemWidget(QWidget* parent = nullptr);

    // "attente" | "vu_a" | "vu_b"
    void setEtat(const QString& etat, qint64 timestampMs);
    // "entree" | "sortie"
    void setDernierPassage(const QString& direction, qint64 timestampMs);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void lancerAnimation();

    QString m_etat = QStringLiteral("attente");
    QString m_derniereDirection;
    qint64 m_dernierPassageMs = 0;
    QTimer m_timerAnimation;
    int m_phaseAnimation = 0;
};