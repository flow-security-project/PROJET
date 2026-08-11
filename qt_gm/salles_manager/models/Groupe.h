#pragma once

#include <QString>

enum class ModeFlux
{
    Multi = 0,   // salles indépendantes : attente estimée si saturé
    Uni = 1      // plusieurs portails interchangeables : redirection
};

struct Groupe
{
    QString id;
    QString nom;
    ModeFlux mode = ModeFlux::Uni;
    double seuilEcart = 0.15;  // écart de taux minimum pour rediriger (anti ping-pong)
};
