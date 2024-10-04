/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2013 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2022 Abhishek Srivastava, University of Minho, 
    João Miguel Nóbrega, University of Minho and João Vidal, Soprefa SA
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::viscosityModels::tempBirdCarreau

Description
    An incompressible Bird-Carreau non-Newtonian viscosity model with Arrhenius Law.

    The Bird-Carreau-Yasuda form is also supported if the optional "a"
    coefficient is specified.  "a" defaults to 2 for the Bird-Carreau model.

SourceFiles
    tempBirdCarreau.C
\*---------------------------------------------------------------------------*/

#include "tempBirdCarreau.H"
#include "addToRunTimeSelectionTable.H"
#include "surfaceFields.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace viscosityModels
{
    defineTypeNameAndDebug(tempBirdCarreau, 0);
    addToRunTimeSelectionTable
    (
        viscosityModel,
        tempBirdCarreau,
        dictionary
    );
}
}


// * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * * //

Foam::tmp<Foam::volScalarField>
Foam::viscosityModels::tempBirdCarreau::calcNu() const
{
    const volScalarField& T=U_.mesh().lookupObject<volScalarField>("T");
    return
      
      (nuInf_
      + (nu0_ - nuInf_)
      *pow(scalar(1) + sqr((k_*strainRate()*exp(a_*((scalar(1)/T)-(scalar(1)/Tbase_))))), (n_ - 1.0)/2.0))*exp(a_*((scalar(1)/T)-(scalar(1)/Tbase_))); 
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::viscosityModels::tempBirdCarreau::tempBirdCarreau
(
    const word& name,
    const dictionary& viscosityProperties,
    const volVectorField& U,
    const surfaceScalarField& phi
)
:
    viscosityModel(name, viscosityProperties, U, phi),
    tempBirdCarreauCoeffs_
    (
        viscosityProperties.subDict(typeName + "Coeffs")
    ),

    nu0_("nu0", dimViscosity, tempBirdCarreauCoeffs_),
    nuInf_("nuInf", dimViscosity, tempBirdCarreauCoeffs_),
    k_("k", dimTime, tempBirdCarreauCoeffs_),
    n_("n", dimless, tempBirdCarreauCoeffs_),
    a_("a", dimTemperature, tempBirdCarreauCoeffs_),
    Tbase_("Tbase", dimTemperature, tempBirdCarreauCoeffs_),
    
    nu_
    (
        IOobject
        (
            "nu",
            U_.time().timeName(),
            U_.db(),
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        calcNu()
    )
{}


// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //

bool Foam::viscosityModels::tempBirdCarreau::read
(
    const dictionary& viscosityProperties
)
{
    viscosityModel::read(viscosityProperties);

    tempBirdCarreauCoeffs_ = viscosityProperties.subDict(typeName + "Coeffs");

    tempBirdCarreauCoeffs_.readEntry("nu0", nu0_);
    tempBirdCarreauCoeffs_.readEntry("nuInf", nuInf_);
    tempBirdCarreauCoeffs_.readEntry("k", k_);
    tempBirdCarreauCoeffs_.readEntry("n", n_);
    tempBirdCarreauCoeffs_.readEntry("a", a_);
    tempBirdCarreauCoeffs_.readEntry("Tbase", Tbase_);
   
    return true;
}


// ************************************************************************* //
