#include "PassageDetectorAB.h"

PassageDetectorAB::PassageDetectorAB(QObject* parent)
    : QObject(parent)
{
}

void PassageDetectorAB::majToF(bool bloque, qint64 tMs)
{
    if (bloque) {
        if (++m_tofBloqueCompte < m_debounce)
            return;
        if (m_tofBloque)
            return; // front déjà compté pour ce blocage
        m_tofBloque = true;
        if (m_etat == Etat::Attente) {
            m_etat = Etat::VuA;
            m_dernierDeclenchementMs = tMs;
            emit capteurAActive();
        } else if (m_etat == Etat::VuB) {
            m_etat = Etat::Attente;
            m_dernierDeclenchementMs = 0;
            emit passageValide(QStringLiteral("sortie"));
        }
    } else {
        m_tofBloque = false;
        m_tofBloqueCompte = 0;
    }
}

void PassageDetectorAB::declencherUltrason(qint64 tMs)
{
    if (m_etat == Etat::Attente) {
        m_etat = Etat::VuB;
        m_dernierDeclenchementMs = tMs;
        emit capteurBActive();
    } else if (m_etat == Etat::VuA) {
        m_etat = Etat::Attente;
        m_dernierDeclenchementMs = 0;
        emit passageValide(QStringLiteral("entree"));
    }
}

void PassageDetectorAB::verifierExpiration(qint64 maintenantMs)
{
    if (m_etat == Etat::Attente)
        return;
    if (maintenantMs - m_dernierDeclenchementMs > m_fenetreMs) {
        m_etat = Etat::Attente;
        m_dernierDeclenchementMs = 0;
        emit sequenceAnnulee();
    }
}

void PassageDetectorAB::reset()
{
    m_etat = Etat::Attente;
    m_dernierDeclenchementMs = 0;
}