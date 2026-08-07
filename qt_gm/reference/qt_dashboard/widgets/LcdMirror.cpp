#include "LcdMirror.h"

#include <QTimer>
#include <QVBoxLayout>

LcdMirror::LcdMirror(QWidget* parent)
    : QFrame(parent)
{
    setObjectName("lcdMirror");
    setFixedSize(260, 70);
    m_l1 = new QLabel("----", this);
    m_l2 = new QLabel("----", this);
    m_l1->setStyleSheet("color:#2E7D32;font-family:monospace;font-size:13px;");
    m_l2->setStyleSheet("color:#2E7D32;font-family:monospace;font-size:13px;");
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(10, 8, 10, 8);
    lay->setSpacing(2);
    lay->addWidget(m_l1);
    lay->addWidget(m_l2);
}

void LcdMirror::afficher(const QString& ligne1, const QString& ligne2)
{
    m_l1->setText(ligne1);
    m_l2->setText(ligne2);
}

void LcdMirror::afficherTemporaire(const QString& ligne1, const QString& ligne2,
                                   int dureeMs)
{
    afficher(ligne1, ligne2);
    QTimer::singleShot(dureeMs, this, [this]() {
        afficher("----", "----");
    });
}
