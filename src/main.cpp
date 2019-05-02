// version 0.1
#include <time.h>
#include <chrono>
#include <iostream>
#include <string>
#include <fstream>
//#include "bdd.h"
//#include "fdd.h"
#include <mpi.h>

#include "DistributedSOG.h"
#include "threadSOG.h"
#include "HybridSOG.h"
#include "LDDGraph.h"


#include <spot/misc/version.hh>
#include <spot/twaalgos/dot.hh>
#include <spot/tl/parse.hh>
#include <spot/tl/print.hh>
#include <spot/twaalgos/translate.hh>
#include <spot/twaalgos/emptiness.hh>
#include <spot/tl/apcollect.hh>
#include "NewNet.h"
#include "SogTwa.h"
#include "SogKripke.h"




// #include "RdPBDD.h"

using namespace std;
#define MASTER 0

int Formula_transitions(const char * f, Set_mot& formula_trans, NewNet Rv) ;
//int nb_th;

unsigned int nb_th;
int n_tasks, task_id;
spot::formula not_f;
set<string> buildPropositions(const string &fileName)
{
    string input;
    set<string> transitionSet;
    ifstream file(fileName);
    if (file.is_open())
    {
        getline (file,input);
        cout<<"Loaded formula : "<<input<<endl;
        file.close();

    }
    else
    {
        cout<<"Can not open formula file"<<endl;
        exit(0);
    }

    spot::parsed_formula pf = spot::parse_infix_psl(input);
    if (pf.format_errors(std::cerr))
        return transitionSet;
    spot::formula fo = pf.f;
    if (!fo.is_ltl_formula())
    {
        std::cerr << "Only LTL formulas are supported.\n";
        return transitionSet;
    }
    spot::atomic_prop_set *p_list=spot::atomic_prop_collect(fo,0);
    for (spot::atomic_prop_set::const_iterator i=p_list->begin(); i!=p_list->end(); i++)
    {
        transitionSet.insert((*i).ap_name());
    }
    print_spin_ltl(std::cout, fo)<<'\n';
    cout<<"Building formula negation\n";
    not_f = spot::formula::Not(pf.f);
    return transitionSet;

}

/***********************************************/
int main(int argc, char** argv)
{
    int choix;
    /*string input=read_formula(argv[4]);

    exit(0);*/

    if(argc<3)
        return 0;
    char formula[100]="";
    char Int[100]="";
    strcpy(formula, argv[4]);
    if (argc > 6)
        strcpy(Int, argv[5]);

    int bound  = atoi(argv[argc - 1])==0?32:atoi(argv[argc - 1]);
    //if(argc>5)
    nb_th = atoi(argv[2])==0 ? 1 : atoi(argv[2]);


    cout<<"Fichier net : "<<argv[3]<<endl;
    cout<<"Fichier formule : "<<formula<<endl;
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
    cout<<"Fetching formula..."<<endl;
    set<string> list_propositions=buildPropositions(formula);

    NewNet R(argv[3],list_propositions);

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
            if (!strcmp(argv[1],"p"))
            {
                cout<<"Construction with pthread library."<<endl;
                cout<<"Count of threads to be created: "<<nb_th<<endl;
                DR.computeDSOG(g,false);
                g.printCompleteInformation();
            }
            else if (!strcmp(argv[1],"pc"))
            {
                cout<<"Canonized construction with pthread library."<<endl;
                cout<<"Count of threads to be created: "<<nb_th<<endl;
                DR.computeDSOG(g,true);
                g.printCompleteInformation();
            }
            else if (!strcmp(argv[1],"l"))
            {
                cout<<"Construction with lace framework."<<endl;
                cout<<"Count of workers to be created: "<<nb_th<<endl;
                DR.computeSOGLace(g);
                g.printCompleteInformation();
            }
            else if (!strcmp(argv[1],"lc"))
            {
                cout<<"Canonised construction with lace framework."<<endl;
                cout<<"Count of workers to be created: "<<nb_th<<endl;
                DR.computeSOGLaceCanonized(g);
                g.printCompleteInformation();
            }

            cout<<"Perform Model checking ?";
            char c;
            cin>>c;
            if (c='y')
            {
                cout<<"Building automata for not(formula)\n";
                auto d = spot::make_bdd_dict();
               // d->register_ap("jbhkj");
                spot::twa_graph_ptr af = spot::translator(d).run(not_f);
                cout<<"Formula automata built.\n";
                cout<<"Want to save the graph in a dot file ?";
                cin>>c;
                if (c=='y') {
                    fstream file;
                    string st(formula);
                    st+=".dot";
                    file.open(st.c_str(),fstream::out);
                    spot::print_dot(file, af);
                    file.close();
                }
                auto k = std::make_shared<SogKripke>(d,DR.getGraph(),R.getListTransitionAP());
                cout<<"SOG translated to SPOT succeeded.."<<endl;
                cout<<"Want to save the graph in a dot file ?";
                cin>>c;
                if (c=='y') {
                    fstream file;
                    string st(argv[3]);
                    st+=".dot";
                    file.open(st.c_str(),fstream::out);
                    spot::print_dot(file, k);
                    file.close();
                }
                if (auto run = k->intersecting_run(af))
                    std::cout << "formula is violated by the following run:\n" << *run;
                else
                    std::cout << "formula is verified\n";
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
