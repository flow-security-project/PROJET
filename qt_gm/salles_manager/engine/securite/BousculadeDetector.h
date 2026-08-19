#pragma once

#include <QObject>
#include <QString>
#include <QMap>

struct Salle;

class BousculadeDetector : public QObject
{
    Q_OBJECT
public:
    struct Resultat
    {
        bool alerte = false;
        bool bousculadeActive = false;
        double dureeS = 0.0;
    };

    struct EtatSalle
    {
        bool active = false;
        qint64 debutMs = 0;
    };

    explicit BousculadeDetector(QObject* parent = nullptr);

    void setSeuils(double tauxSaturation = 0.95, double debitSortieSeuil = 15.0, int dureeMinS = 10);

    Resultat verifier(const Salle& salle, qint64 maintenantMs);
    void reset();

signals:
    void bousculadeDetectee(const QString& salleId, double dureeS);

private:
    double m_tauxSaturation = 0.95;
    double m_debitSortieSeuil = 15.0;
    int m_dureeMinS = 10;

    QMap<QString, EtatSalle> m_etats;
};