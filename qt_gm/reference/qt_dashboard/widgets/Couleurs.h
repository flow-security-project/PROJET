#pragma once

#include <QColor>
#include <QString>

inline QColor couleurLedHex(const QString& led)
{
    if (led == "vert")    return QColor("#2E7D32");
    if (led == "jaune")   return QColor("#FBC02D");
    if (led == "orange")  return QColor("#F57C00");
    if (led == "rouge")   return QColor("#C62828");
    if (led == "bleu")    return QColor("#1565C0");
    return QColor("#616161"); // gris
}

inline QString severiteHex(const QString& sev)
{
    if (sev == "critique") return "#C62828";
    if (sev == "info")     return "#FBC02D";
    return "#F57C00"; // attention
}

inline QString severiteObjectName(const QString& sev)
{
    if (sev == "critique") return "badgeCritique";
    if (sev == "info")     return "badgeInfo";
    return "badgeAttention";
}
