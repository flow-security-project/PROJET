#pragma once

#include <QFrame>
#include <QLabel>

class LcdMirror : public QFrame
{
    Q_OBJECT

public:
    explicit LcdMirror(QWidget* parent = nullptr);

    void afficher(const QString& ligne1, const QString& ligne2);
    void afficherTemporaire(const QString& ligne1, const QString& ligne2,
                            int dureeMs);

private:
    QLabel* m_l1 = nullptr;
    QLabel* m_l2 = nullptr;
};
