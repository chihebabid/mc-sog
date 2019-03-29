// version 0.1
#include <time.h>
#include <chrono>
#include <iostream>
#include <string>
//#include "bdd.h"
//#include "fdd.h"
#include <mpi.h>
#include "DistributedSOG.h"
#include "threadSOG.h"
#include "HybridSOG.h"
#include "LDDGraph.h"
#include "Net.hpp"
// #include "RdPBDD.h"

using namespace std;
#define MASTER 0

int Formula_transitions(const char * f, Set_mot& formula_trans, net Rv) ;
//int nb_th;

unsigned int nb_th;
int n_tasks, task_id;

/***********************************************/
int main(int argc, char** argv)
{
    int choix;

    //nb_th;
    if(argc<3)  return 0;
    char Obs[100]="";
    char Int[100]="";
    if (argc > 5) strcpy(Obs, argv[4]);
    if (argc > 6) strcpy(Int, argv[5]);

    int bound  = atoi(argv[argc - 1])==0?32:atoi(argv[argc - 1]);
    //if(argc>5)
    nb_th = atoi(argv[2])==0 ? 1 : atoi(argv[2]);


    cout<<"Fichier net : "<<argv[3]<<endl;
    cout<<"Fichier formule : "<<Obs<<endl;
    cout<<"Fichier Interface : "<<Int<<endl;
    cout<<"Bound : "<<argv[argc-1]<<endl;
    cout<<"thread : "<<argv[2]<<endl;
    //strcpy(red,argv[5]);
    //cout<<"Reduce : "<<red<<endl;

    if(nb_th==0)
    {
        cout<<"number of thread <= 0 " <<endl;
        exit (0);
    }

    cout<<"______________________________________\n";
    net R(argv[3],Obs,Int);

    MPI_Init (&argc, &argv );
    MPI_Comm_size(MPI_COMM_WORLD,&n_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&task_id);



    //
    if (n_tasks==1)
    {
        cout<<"number of task = 1 \n " <<endl;
        bool uselace=(!strcmp(argv[1],"lc")) || (!strcmp(argv[1],"l"));
        threadSOG DR(R, bound,nb_th,uselace);
        LDDGraph g;

        if (nb_th==1)
        {
            cout<<"******************Sequential version******************* \n" <<endl;
            DR.computeSeqSOG(g);
            g.printCompleteInformation();
        }
        else
        {
            cout<<"*******************Multithread version****************** \n" <<endl;
            if (!strcmp(argv[1],"p")) {
                cout<<"Construction with pthread library."<<endl;
                cout<<"Count of threads to be created: "<<nb_th<<endl;
                DR.computeDSOG(g,false);
                g.printCompleteInformation();
            }
            else if (!strcmp(argv[1],"pc")) {
                cout<<"Canonized construction with pthread library."<<endl;
                cout<<"Count of threads to be created: "<<nb_th<<endl;
                DR.computeDSOG(g,true);
                g.printCompleteInformation();
            }
            else if (!strcmp(argv[1],"l")) {
                cout<<"Construction with lace framework."<<endl;
                cout<<"Count of workers to be created: "<<nb_th<<endl;
                DR.computeSOGLace(g);
                g.printCompleteInformation();
            }
             else if (!strcmp(argv[1],"lc")) {
                cout<<"Canonised construction with lace framework."<<endl;
                cout<<"Count of workers to be created: "<<nb_th<<endl;
                DR.computeSOGLaceCanonized(g);
                g.printCompleteInformation();
            }
        }
    }


    if (n_tasks>1)
    {
        if(nb_th>1)
        {
            cout<<"**************Hybrid version**************** \n" <<endl;
            HybridSOG DR(R, bound);

            LDDGraph g;
            DR.computeDSOG(g);
        }
        else
        {
            //cout<<" sequential version using Sylvan : 1 with BuDDy : 2 \n" <<endl;
            cout<<"*************Distibuted version******************* \n" <<endl;

            {
                DistributedSOG DR(R, bound);
                LDDGraph g;
                DR.computeDSOG(g);
            }


        }
    }

    MPI_Finalize();
    return 0;
}
