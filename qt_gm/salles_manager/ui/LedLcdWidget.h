#pragma once

#include <QWidget>

// Visualisation de la LED progressive F1 et du miroir LCD 16x2 F8
class LedLcdWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LedLcdWidget(QWidget* parent = nullptr);

    void setAffichage(const QString& ledColor, const QString& ledColorConf, const QString& ledMode,
                      const QString& l1, const QString& l2, bool enLigne, bool enAttente);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_ledCouleur = QStringLiteral("eteint");
    QString m_ledCouleurConf = QStringLiteral("eteint");
    QString m_ledMode = QStringLiteral("normal");
    QString m_lcdLigne1;
    QString m_lcdLigne2;
    bool m_enLigne = false;
    bool m_enAttente = false;
};
