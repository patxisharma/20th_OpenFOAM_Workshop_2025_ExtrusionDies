Info<< "Reading field p\n" << endl;
volScalarField p
(
    IOobject
    (
        "p",
        runTime.timeName(),
        mesh,
        IOobject::MUST_READ,
        IOobject::AUTO_WRITE
    ),
    mesh
);

Info<< "Reading field U\n" << endl;
volVectorField U
(
    IOobject
    (
        "U",
        runTime.timeName(),
        mesh,
        IOobject::MUST_READ,
        IOobject::AUTO_WRITE
    ),
    mesh
);


Info<< "Reading field T\n" << endl;
volScalarField T
(
    IOobject
    (
        "T",
        runTime.timeName(),
        mesh,
        IOobject::MUST_READ,
        IOobject::AUTO_WRITE
    ),
    mesh
);

Info<< "Reading transportProperties\n" << endl;
IOdictionary transportProperties
(
 IOobject
 (
 "transportProperties",
 runTime.constant(),
 mesh,
 IOobject::MUST_READ,
 IOobject::NO_WRITE
 )
);

//const Foam::dictionary& transportPropertiesDict = transportProperties;
const Foam::dimensionedScalar DT = Foam::dimensionedScalar
(
    "DT",
    transportProperties.subDict("DT")  // Use subDict() to get the dictionary entry
);


const Foam::dimensionedScalar cp = Foam::dimensionedScalar
(
    "cp",
    transportProperties.subDict("cp")  // Use subDict() to get the dictionary entry
);

#include "createPhi.H"


label pRefCell = 0;
scalar pRefValue = 0.0;
setRefCell(p, simple.dict(), pRefCell, pRefValue);
mesh.setFluxRequired(p.name());


singlePhaseTransportModel laminarTransport(U, phi);

autoPtr<incompressible::turbulenceModel> turbulence
(
    incompressible::turbulenceModel::New(U, phi, laminarTransport)
);

#include "createMRF.H"
#include "createFvOptions.H"
