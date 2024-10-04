//Energy equation

   	volTensorField gradU = fvc::grad(U);
	volScalarField nu = turbulence -> nu();
	volTensorField tau = nu * (gradU + gradU.T()); 
	
	fvScalarMatrix TEqn
	(
		fvm::div(phi , T)
		
		- fvm::laplacian(DT , T)
		
		== (1/cp)*(tau && gradU)
	);
    
	TEqn.relax();
    
        TEqn.solve();
	  