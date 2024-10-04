/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2022 AUTHOR,AFFILIATION
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.
    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU Gene forAll(fobjTot,cellI)
    {
        scalar fobjTot = fobjTot.value();
    }ral Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.
    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.
\*---------------------------------------------------------------------------*/

#include "fobjArea.H"
#include "Time.H"
#include "runTimeControl.H"
#include "OSspecific.H" // getEnv();
#include "IFstream.H"
#include "addToRunTimeSelectionTable.H"
#include "OFstream.H"


// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    namespace functionObjects
    {
        defineTypeNameAndDebug(fobjArea, 0);
        addToRunTimeSelectionTable(functionObject, fobjArea, dictionary);
    }
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::functionObjects::fobjArea::fobjArea(
    const word &name,
    const Time &runTime,
    const dictionary &dict)
    : fvMeshFunctionObject(name, runTime, dict),
      nES_(dict.get<label>("nES")),
      nIS_(dict.get<label>("nIS")),
      nCounter_(),
      stepSize_(dict.get<label>("stepSize")),
      nOldTimeStep_(0),
      fobjOld_(0),
      diff_(1),
      tolerance_(dict.get<scalar>("tolerance")),
      minCounter_(dict.get<label>("minCounter"))
{
    // Create the output path directory
    fileName outputDir = mesh_.time().globalPath() / "postProcessing";

    // Create the directory
    mkDir(outputDir);

    // Open the file in the newly created directory
    outputFilePtr_.reset(new OFstream(outputDir / "Fobj.dat"));

    // Write
    outputFilePtr_() << "TimeStep "
                     << ",";
    outputFilePtr_() << "nES"
                     << " ,";
    outputFilePtr_() << "nIS"
                     << ", ";

    for (label i = 1; i <= nES_; i++)
    {
        outputFilePtr_() << "Fobj_ES" << i;
        if (i < nES_)
            outputFilePtr_() << ", ";
    }
    for (label i = 1; i <= nIS_; i++)
    {
        outputFilePtr_() << ", "
                         << "Fobj_IS" << i;
        if (i < nIS_)
            outputFilePtr_();
    }

    outputFilePtr_() << ","
                     << " Fobj_Tot , avVeL" << endl;
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::functionObjects::fobjArea::read(const dictionary &dict)
{
    // dict.readEntry("boolData", boolData_);
    dict.readEntry("nES", nES_);
    dict.readEntry("nIS", nIS_);
    for (label i = 1; i <= (nES_ + nIS_); i++)
    {
        word nPatch;
        if (i <= nES_)
        {
            nPatch = "ES";
            nPatch += Foam::name(i);
        }
        else
        {
            nPatch = "IS";
            nPatch += Foam::name(i - nES_);
        }
        areaESIS_.append(dict.get<scalar>(nPatch));
    }
    return true;
}

bool Foam::functionObjects::fobjArea::execute()
{
    return true;
}

bool Foam::functionObjects::fobjArea::end()
{
    return true;
}

bool Foam::functionObjects::fobjArea::write()
{
    const Foam::dictionary &controlDict_ = time_.controlDict();
    const surfaceScalarField &phi_ = mesh_.lookupObject<surfaceScalarField>("phi");

    DynamicList<scalar> flowESIS;
    DynamicList<scalar> fobjESIS;
    DynamicList<scalar> areaESIS;

    scalar flowTot = 0.0;
    scalar areaTot = 0.0;

    for (label i = 1; i <= (nES_ + nIS_); i++)
    {
        word nPatch;
        if (i <= nES_)
        {
            nPatch = "ES";
            nPatch += Foam::name(i);
        }
        else
        {
            nPatch = "IS";
            nPatch += Foam::name(i - nES_);
        }
        label patchID = mesh_.boundaryMesh().findPatchID(nPatch);
        if (patchID < 0)
        {
            FatalErrorInFunction
                << "Unable to find patch " << nPatch
                << abort(FatalError);
        }
        areaESIS_.append(controlDict_.get<scalar>(nPatch));
        scalar flowPatch = mag(gSum(phi_.boundaryField()[patchID]));
        flowESIS.append(mag(gSum(phi_.boundaryField()[patchID])));
        flowTot += flowPatch;
        areaTot += controlDict_.get<scalar>(nPatch);
    }

    scalar aveVel = flowTot / areaTot;
    scalar fobjTot = 0.0;
    scalar fobjESISi = 0.0;
    for (label i = 0; i < (nES_ + nIS_); i++)
    {
        fobjESISi = (flowESIS[i] / (flowTot * (areaESIS_[i] / areaTot)) - 1) / max((flowESIS[i] / (flowTot * (areaESIS_[i] / areaTot))), 1.0);
        fobjESIS.append(fobjESISi);
        fobjTot += mag(fobjESISi * (areaESIS_[i] / areaTot));
    }

    if (Pstream::master())
    {
        outputFilePtr_() << time_.time().value() << ", ";
        outputFilePtr_() << nES_ << ", ";
        outputFilePtr_() << nIS_ << ", ";

        for (label i = 0; i < (nES_ + nIS_); i++)
        {
            outputFilePtr_() << fobjESIS[i] << i;
            if (i < (nES_ + nIS_ - 1))
                outputFilePtr_() << ", ";
        }

        outputFilePtr_() << ", " << fobjTot << ", " << aveVel << endl;
    }

    if (Pstream::master())
    {
        
        scalar timeStep = time_.time().value(); 
        diff_ = mag((fobjOld_ - fobjTot) /max(fobjOld_,fobjTot));
        if (timeStep < stepSize_ + nOldTimeStep_)
        {
            Info << "waiting for convergence check " << endl;
            Info << "Difference from fobjTot prev value is: " << diff_ << endl;
            nCounter_=0;
        }
        else
        {
            if (diff_ < tolerance_)
            {
                if (nCounter_ < minCounter_ )
                {
                    Info << "Difference from fobjTot prev value is: " << diff_ << endl;
                    Info << "nCounter: " << nCounter_ << endl;
                    nCounter_ += 1;
                }
                else
                {
                    Time &runTime = const_cast<Time &>(mesh_.time());
                    Info << "Difference from fobjTot prev value is: " << diff_ << endl;
                    Info << "Reached convergence criterion for fobjTot " << endl;
                    Info << "Fobj value is " << fobjTot  << endl;
                    Info << "Solution converged in " << runTime.timeName() << " iterations" << nl << endl;
                    runTime.printExecutionTime(Info);
                    runTime.writeAndEnd();
                }
            }
            else
            {
                Info << "Not converged in " << minCounter_  << " iterations" << endl;
                Info << "Difference from fobjTot prev value is: " << diff_ << endl;
                nOldTimeStep_=time_.time().value();
            }
            fobjOld_ = fobjTot;
        }
    }
    return true;
}