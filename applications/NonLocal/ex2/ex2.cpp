
#include "FemusInit.hpp"
#include "MultiLevelProblem.hpp"
#include "VTKWriter.hpp"
#include "TransientSystem.hpp"
#include "NonLinearImplicitSystem.hpp"

#include "NumericVector.hpp"
#include "adept.h"

#include "petsc.h"
#include "petscmat.h"
#include "PetscMatrix.hpp"

#include "slepceps.h"

#include "../include/nonlocal_assembly1.hpp"


//SECOND NONLOCAL EX IN FEMUS: nonlocal diffusion for a body with different material properties (this works with a different assembly than ex1)

using namespace femus;

double InitalValueU ( const std::vector < double >& x )
{
//     return x[0] + 0. * ( 0.51 * 0.51 - x[0] * x[0] ) * ( 0.51 * 0.51 - x[1] * x[1] );
//     return x[0];
    return x[0] * x[0];
//     return x[0] * x[0] * x[0] + x[1] * x[1] * x[1];
//     return x[0] * x[0] * x[0] * x[0] + 0.8 * x[0] * x[0]; //this is if delta = 0.2
//     return x[0] * x[0] * x[0] * x[0]; //this is x^4
//        return 2 * x[0] + x[0] * x[0] * x[0] * x[0] * x[0]; //this is 2x + x^5
}

void GetL2Norm ( MultiLevelProblem& ml_prob );

bool SetBoundaryCondition ( const std::vector < double >& x, const char SolName[], double& value, const int facename, const double time )
{

    bool dirichlet = true;
//     value = 0.;
//     value = x[0];
    value = x[0] * x[0];
//     value = x[0] * x[0] * x[0] + x[1] * x[1] * x[1];
//     value = x[0] * x[0] * x[0] * x[0] + 0.8 * x[0] * x[0]; //this is if delta = 0.2
//     value = x[0] * x[0] * x[0] * x[0];
//        value =  2 * x[0] + x[0] * x[0] * x[0] * x[0] * x[0]; //this is 2x + x^5

    if ( facename == 2 ) {
        dirichlet = false; //Neumann at the interface boundaries
        value = 0.;
    }

    return dirichlet;
}

unsigned numberOfUniformLevels = 3;

int main ( int argc, char** argv )
{


    // init Petsc-MPI communicator
    FemusInit mpinit ( argc, argv, MPI_COMM_WORLD );

    MultiLevelMesh mlMsh;
    double scalingFactor = 1.;
    unsigned numberOfSelectiveLevels = 0;
//     mlMsh.ReadCoarseMesh ( "../input/nonlocal_boundary_test.neu", "second", scalingFactor );
//     mlMsh.ReadCoarseMesh ( "../input/interface.neu", "second", scalingFactor );
//     mlMsh.ReadCoarseMesh ( "../input/maxTest1.neu", "second", scalingFactor );
//     mlMsh.ReadCoarseMesh ( "../input/maxTest2.neu", "second", scalingFactor );
//         mlMsh.ReadCoarseMesh ( "../input/maxTest3.neu", "second", scalingFactor );
//     mlMsh.ReadCoarseMesh ( "../input/maxTest4.neu", "second", scalingFactor );
//         mlMsh.ReadCoarseMesh ( "../input/maxTest5.neu", "second", scalingFactor );
//     mlMsh.ReadCoarseMesh ( "../input/maxTest6.neu", "second", scalingFactor );
//     mlMsh.ReadCoarseMesh ( "../input/maxTest7.neu", "second", scalingFactor );
//     mlMsh.ReadCoarseMesh ( "../input/maxTest2Continuous.neu", "second", scalingFactor );
//     mlMsh.ReadCoarseMesh ( "../input/martaTest1.neu", "second", scalingFactor );
    mlMsh.ReadCoarseMesh ( "../input/martaTest2.neu", "second", scalingFactor );
//     mlMsh.ReadCoarseMesh ( "../input/martaTest3.neu", "second", scalingFactor );
//        mlMsh.ReadCoarseMesh ( "../input/martaTest4.neu", "second", scalingFactor );
//     mlMsh.ReadCoarseMesh ( "../input/martaTest4Coarser.neu", "second", scalingFactor );
//     mlMsh.ReadCoarseMesh ( "../input/trial1.neu", "second", scalingFactor );
//     mlMsh.ReadCoarseMesh ( "../input/trial2.neu", "second", scalingFactor );
    mlMsh.RefineMesh ( numberOfUniformLevels + numberOfSelectiveLevels, numberOfUniformLevels , NULL );

    mlMsh.EraseCoarseLevels ( numberOfUniformLevels - 1 );
//     numberOfUniformLevels = 1;

    unsigned dim = mlMsh.GetDimension();

    MultiLevelSolution mlSol ( &mlMsh );

    // add variables to mlSol
    mlSol.AddSolution ( "u", LAGRANGE, FIRST, 2 );
    
    mlSol.AddSolution ( "u_exact", LAGRANGE, FIRST, 2 );

    mlSol.Initialize ( "All" );

    mlSol.Initialize("u_exact", InitalValueU);

    mlSol.AttachSetBoundaryConditionFunction ( SetBoundaryCondition );

    // ******* Set boundary conditions *******
    mlSol.GenerateBdc ( "All" );

    MultiLevelProblem ml_prob ( &mlSol );

    // ******* Add FEM system to the MultiLevel problem *******
    LinearImplicitSystem& system = ml_prob.add_system < LinearImplicitSystem > ( "NonLocal" );
    system.AddSolutionToSystemPDE ( "u" );

    // ******* System FEM Assembly *******
    system.SetAssembleFunction ( AssembleNonLocalSys );
    system.SetMaxNumberOfLinearIterations ( 1 );
    // ******* set MG-Solver *******
    system.SetMgType ( V_CYCLE );

    system.SetAbsoluteLinearConvergenceTolerance ( 1.e-50 );
    //   system.SetNonLinearConvergenceTolerance(1.e-9);
//   system.SetMaxNumberOfNonLinearIterations(20);

    system.SetNumberPreSmoothingStep ( 1 );
    system.SetNumberPostSmoothingStep ( 1 );

    // ******* Set Preconditioner *******
    system.SetMgSmoother ( GMRES_SMOOTHER );

    system.SetSparsityPatternMultiplyingFactor ( 500u ); //TODO tune

    system.init();

    // ******* Set Smoother *******
    system.SetSolverFineGrids ( GMRES );

    system.SetPreconditionerFineGrids ( ILU_PRECOND );

    system.SetTolerances ( 1.e-20, 1.e-20, 1.e+50, 100 );

// ******* Solution *******

    system.MGsolve();

    GetL2Norm ( ml_prob );

    // ******* Print solution *******
    mlSol.SetWriter ( VTK );
    std::vector<std::string> print_vars;
    print_vars.push_back ( "All" );
    mlSol.GetWriter()->SetDebugOutput ( true );
    mlSol.GetWriter()->Write ( DEFAULT_OUTPUTDIR, "biquadratic", print_vars, 0 );

    return 0;

} //end main




void GetL2Norm ( MultiLevelProblem& ml_prob )
{

    LinearImplicitSystem* mlPdeSys  = &ml_prob.get_system<LinearImplicitSystem> ( "NonLocal" );
    const unsigned level = mlPdeSys->GetLevelToAssemble();
    Mesh*                    msh = ml_prob._ml_msh->GetLevel ( level );
    elem*                     el = msh->el;
    MultiLevelSolution*    mlSol = ml_prob._ml_sol;
    Solution*                sol = ml_prob._ml_sol->GetSolutionLevel ( level );

    const unsigned  dim = msh->GetDimension();

    unsigned xType = 2; // get the finite element type for "x", it is always 2 (LAGRANGE QUADRATIC)

    double local_norm2 = 0.;

    double sol_norm2 = 0.;

    double sol_exact_norm2 = 0.;

    unsigned soluIndex;
    soluIndex = mlSol->GetIndex ( "u" );
    unsigned soluType = mlSol->GetSolutionType ( soluIndex );


    unsigned    iproc = msh->processor_id();

    for ( int iel = msh->_elementOffset[iproc]; iel < msh->_elementOffset[iproc + 1]; iel++ ) {

        short unsigned ielGeom = msh->GetElementType ( iel );
        unsigned nDofu  = msh->GetElementDofNumber ( iel, soluType );
        unsigned nDofx = msh->GetElementDofNumber ( iel, xType );

        vector < vector < double > > x1 ( dim );

        for ( int i = 0; i < dim; i++ ) {
            x1[i].resize ( nDofx );
        }

        vector < double >  soluNonLoc ( nDofu );

        for ( unsigned i = 0; i < nDofu; i++ ) {
            unsigned solDof = msh->GetSolutionDof ( i, iel, soluType );
            soluNonLoc[i] = ( *sol->_Sol[soluIndex] ) ( solDof );
        }

        for ( unsigned i = 0; i < nDofx; i++ ) {
            unsigned xDof  = msh->GetSolutionDof ( i, iel, xType );

            for ( unsigned jdim = 0; jdim < dim; jdim++ ) {
                x1[jdim][i] = ( *msh->_topology->_Sol[jdim] ) ( xDof );
            }
        }

        vector <double> phi;  // local test function
        vector <double> phi_x; // local test function first order partial derivatives
        double weight; // gauss point weight

        // *** Gauss point loop ***
        for ( unsigned ig = 0; ig < msh->_finiteElement[ielGeom][soluType]->GetGaussPointNumber(); ig++ ) {
            // *** get gauss point weight, test function and test function partial derivatives ***
            msh->_finiteElement[ielGeom][soluType]->Jacobian ( x1, ig, weight, phi, phi_x );
            double soluNonLoc_gss = 0.;
            double exactSol_gss_x = 0.;
            double exactSol_gss_y = 0.;


            for ( unsigned i = 0; i < nDofu; i++ ) {
                soluNonLoc_gss += phi[i] * soluNonLoc[i];
                exactSol_gss_x += phi[i] * x1[0][i]; // this is x at the Gauss point
//                 exactSol_gss_y += phi[i] * x1[1][i]; // this is y at the Gauss point
            }

//             exactSol_gss_x = exactSol_gss_x * exactSol_gss_x * exactSol_gss_x * exactSol_gss_x + 0.8 * exactSol_gss_x * exactSol_gss_x; // this is x^4 + 0.4 * x^2

            exactSol_gss_x = exactSol_gss_x * exactSol_gss_x; // this is x^2

//             exactSol_gss_x = exactSol_gss_x * exactSol_gss_x * exactSol_gss_x; // this is x^3
//             exactSol_gss_y = exactSol_gss_y * exactSol_gss_y * exactSol_gss_y; // this is y^3

//             exactSol_gss_x = exactSol_gss_x * exactSol_gss_x * exactSol_gss_x * exactSol_gss_x; // this is x^4

//             exactSol_gss_x = 2 * exactSol_gss_x  + exactSol_gss_x * exactSol_gss_x * exactSol_gss_x * exactSol_gss_x * exactSol_gss_x ; // this is 2x + x^5
            

            local_norm2 += ( soluNonLoc_gss - exactSol_gss_x ) * ( soluNonLoc_gss - exactSol_gss_x ) * weight;

//             local_norm2 += (soluNonLoc_gss -  (exactSol_gss_x + exactSol_gss_y)) * (soluNonLoc_gss -  (exactSol_gss_x + exactSol_gss_y)) * weight; //error L2 norm of x^3 + y^3

            sol_norm2 += soluNonLoc_gss * soluNonLoc_gss * weight;

//             sol_exact_norm2 += (exactSol_gss_x + exactSol_gss_y) * (exactSol_gss_x + exactSol_gss_y) * weight; //L2 norm of x^3 + y^3

            sol_exact_norm2 += exactSol_gss_x * exactSol_gss_x * weight;
        }
    }

    double norm2 = 0.;
    MPI_Allreduce ( &local_norm2, &norm2, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD );
    double norm = sqrt ( norm2 );
    std::cout.precision ( 14 );
    std::cout << "Error L2 norm = " << norm << std::endl;

    norm2 = 0.;
    MPI_Allreduce ( &sol_norm2, &norm2, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD );
    norm = sqrt ( norm2 );
    std::cout.precision ( 14 );
    std::cout << "Sol L2 norm = " << norm << std::endl;

    norm2 = 0.;
    MPI_Allreduce ( &sol_exact_norm2, &norm2, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD );
    norm = sqrt ( norm2 );
    std::cout.precision ( 14 );
    std::cout << "Sol Exact L2 norm = " << norm << std::endl;


}






