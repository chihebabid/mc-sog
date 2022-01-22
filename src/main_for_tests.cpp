#include <time.h>
#include <chrono>
#include <iostream>
#include <string>
#include <fstream>
#include <mpi.h>


#include "LDDGraph.h"
#include "MCMultiCore/ModelCheckerCPPThread.h"
#include "MCMultiCore/ModelCheckerTh.h"
#include "MCMultiCore/ModelCheckThReq.h"
#include "threadSOG.h"
#include "HybridSOG.h"
#include "Hybrid/MCHybrid/MCHybridSOG.h"
#include <spot/misc/version.hh>
#include <spot/twaalgos/dot.hh>
#include <spot/tl/parse.hh>
#include <spot/tl/print.hh>
#include <spot/twaalgos/translate.hh>
#include <spot/twaalgos/emptiness.hh>
#include <spot/tl/apcollect.hh>
#include <spot/ta/taproduct.hh>
#include <spot/twa/twaproduct.hh>
#include <spot/twaalgos/gtec/gtec.hh>
#include "NewNet.h"
#include "SogTwa.h"
#include "SogKripke.h"

#include "SogKripkeTh.h"
#include "Hybrid/HybridKripke.h"
#include "Hybrid/MCHybridReq/MCHybridSOGReq.h"
#include "SylvanWrapper.h"
#include "PMCSOGConfig.h"
// #include "RdPBDD.h"

using namespace std;


unsigned int nb_th;
int n_tasks, task_id;
spot::formula not_f;
set<string> buildPropositions(const string &fileName) {
	string input;
	set<string> transitionSet;
	ifstream file(fileName);
	if (file.is_open()) {
		getline(file, input);
		cout << "Loaded formula : " << input << endl;
		file.close();

	} else {
		cout << "Can not open formula file" << endl;
		exit(0);
	}

	spot::parsed_formula pf = spot::parse_infix_psl(input);
	if (pf.format_errors(std::cerr))
		return transitionSet;
	spot::formula fo = pf.f;
	if (!fo.is_ltl_formula()) {
		std::cerr << "Only LTL formulas are supported.\n";
		return transitionSet;
	}
	spot::atomic_prop_set *p_list = spot::atomic_prop_collect(fo, 0);
	for (spot::atomic_prop_set::const_iterator i = p_list->begin(); i != p_list->end(); i++) {
		transitionSet.insert((*i).ap_name());
	}
	print_spin_ltl(std::cout, fo) << '\n';
	cout << "Building formula negation\n";
	not_f = spot::formula::Not(pf.f);
	return transitionSet;

}
void displayCheckResult(bool res) {
	if (res)
		cout << "Property is verified..." << endl;
	else
		cout << "Property is violated..." << endl;
}

void displayTime(auto startTime,auto finalTime) {
	cout << "Verification duration : " << std::chrono::duration_cast < std::chrono::milliseconds> (finalTime - startTime).count() << " milliseconds\n";
}
/***********************************************/
int main(int argc, char **argv) {
    uint32_t marq[] {1,5,8};
    uint32_t marq2[] {1,12,7};
    uint32_t trans[] {1,5,13};
    SylvanWrapper::sylvan_set_limits(16LL << 30, 10, 0);
    SylvanWrapper::sylvan_init_package();
    SylvanWrapper::sylvan_init_ldd();
    SylvanWrapper::init_gc_seq();
    SylvanWrapper::lddmc_refs_init();
    MDD m=SylvanWrapper::lddmc_cube(marq, 3);
    MDD m2=SylvanWrapper::lddmc_cube(marq2,3);
    m=SylvanWrapper::lddmc_union_mono(m,m2);
    MDD minus=SylvanWrapper::lddmc_cube(trans, 3);
    //SylvanWrapper::isFirable(m,minus);
    std::cout<<SylvanWrapper::isFirable(m,minus)<<endl;
    return 0;
}
int main2(int argc, char **argv) {
	int choix;
	if (argc < 3)
		return 0;
	char formula[100] = "";
	char algorithm[100] = "";
	strcpy(formula, argv[4]);
	if (argc > 5) {
		strcpy(algorithm, argv[5]);
	}

	nb_th = atoi(argv[2]) == 0 ? 1 : atoi(argv[2]);
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &n_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &task_id);
    if (!task_id) {
        cout << "PMC-SOG : Parallel Model Checking tool based on Symbolic Observation Graphs " << endl;
        cout << "Version " <<PMCSOG_VERSION_MAJOR<<"."<<PMCSOG_VERSION_MINOR<<"."<<PMCSOG_VERSION_PATCH<<endl;
        cout << "(c) 2018 - 2020"<<endl;
        cout << "Net file : " << argv[3] << endl;
        cout << "Formula file : " << formula << endl;
        cout << "Checking algorithm : " << algorithm << endl;
        cout << "#threads : " << nb_th << endl;
    }
	if (nb_th == 0) {
		cout << "number of thread <= 0 " << endl;
		exit(0);
	}

	cout << "______________________________________\n";
	cout << "Fetching formula..." << endl;
	set<string> list_propositions = buildPropositions(formula);
	NewNet Rnewnet(argv[3], list_propositions);


    if (n_tasks == 1) {
        if (n_tasks == 1 && (!strcmp(argv[1], "otfPR") || !strcmp(argv[1], "otfC") || !strcmp(argv[1], "otfP"))) {
            cout << "Performing on the fly Model checking..." << endl;
            if (!strcmp(argv[1], "otfP"))
                cout << "Multi-threaded algorithm based on Pthread library!" << endl;
            else if (!strcmp(argv[1], "otfPR"))
                cout << "Multi-threaded algorithm (progressive) based on PThread!" << endl;
            else
                cout << "Multi-threaded algorithm based on C++ Thread library!" << endl;
            cout << "Building automata for not(formula)\n";
            auto d = spot::make_bdd_dict();
            spot::translator obj=spot::translator(d);

            spot::twa_graph_ptr af = obj.run(not_f);
            cout << "Formula automata built.\n";
            ModelCheckBaseMT *mcl;

            if (!strcmp(argv[1], "otfP"))
                mcl = new ModelCheckerTh(Rnewnet, nb_th);
            else if (!strcmp(argv[1], "otfC"))
                mcl = new ModelCheckerCPPThread(Rnewnet, nb_th);
            else
                mcl=new ModelCheckThReq(Rnewnet, nb_th);
            mcl->loadNet();
            auto k = std::make_shared<SogKripkeTh>(d, mcl, Rnewnet.getListTransitionAP(), Rnewnet.getListPlaceAP());
            cout << "Performing on the fly Modelchecking" << endl;
            if (strcmp(algorithm, "")) {
                cout << "Spot emptiness check algorithm : "<<algorithm<< endl;
                shared_ptr < spot::twa_product > product = make_shared<spot::twa_product>(k,af);
                //spot::couvreur99_check_shy check = spot::couvreur99_check_shy(product);
                /****************************/
                const char *err;
                spot::emptiness_check_instantiator_ptr echeck_inst = spot::make_emptiness_check_instantiator(algorithm, &err);

                if (!echeck_inst) {
                    cerr << "Failed to parse argument near `" << err << "'" << endl;
                    cerr << "Spot unknown emptiness algorithm" << endl;
                    exit(2);
                }
                auto startTime = std::chrono::steady_clock::now();
                spot::emptiness_check_ptr echptr = echeck_inst->instantiate(product);
                bool res = (echptr->check() == 0);
                auto finalTime = std::chrono::steady_clock::now();
                displayTime(startTime, finalTime);
                displayCheckResult(res);

            } else {
                auto startTime = std::chrono::steady_clock::now();
                bool res = (k->intersecting_run(af) == 0);
                auto finalTime = std::chrono::steady_clock::now();
                displayTime(startTime, finalTime);
                displayCheckResult(res);
            }
            mcl->finish();
            cout<<"Number of built aggregates: "<<mcl->getGraph()->m_GONodes.size()<<endl;
            delete mcl;
        }

        else {
            cout << "number of task = 1 \n " << endl;
            bool uselace = (!strcmp(argv[1], "lc")) || (!strcmp(argv[1], "l"));
            threadSOG DR(Rnewnet, nb_th, uselace);
            LDDGraph g(&DR);

            if (nb_th == 1) {
                cout << "******************Sequential version******************* \n" << endl;
                DR.computeSeqSOG(g);
                g.printCompleteInformation();
            } else {
                cout << "*******************Multithread version****************** \n" << endl;
                if (!strcmp(argv[1], "p")) {
                    cout << "Construction with pthread library." << endl;
                    cout << "Count of threads to be created: " << nb_th << endl;
                    DR.computeDSOG(g, false);
                    g.printCompleteInformation();
                } else if (!strcmp(argv[1], "pc")) {
                    cout << "Canonized construction with pthread library." << endl;
                    cout << "Count of threads to be created: " << nb_th << endl;
                    DR.computeDSOG(g, true);
                    g.printCompleteInformation();
                }

                cout << "Perform Model checking ?";
                char c;
                cin >> c;
                if (c == 'y') {
                    cout << "Building automata for not(formula)\n";
                    auto d = spot::make_bdd_dict();

                    spot::twa_graph_ptr af = spot::translator(d).run(not_f);
                    cout << "Formula automata built.\n";
                    cout << "Want to save the graph in a dot file ?";
                    cin >> c;
                    if (c == 'y') {
                        fstream file;
                        string st(formula);
                        st += ".dot";
                        file.open(st.c_str(), fstream::out);
                        spot::print_dot(file, af);
                        file.close();
                    }
                    //auto k = std::make_shared<SogKripke>(d,DR.getGraph(),R.getListTransitionAP(),R.getListPlaceAP());

                    spot::twa_graph_ptr k = spot::make_twa_graph(
                            std::make_shared<SogKripke>(d, DR.getGraph(), Rnewnet.getListTransitionAP(), Rnewnet.getListPlaceAP()),
                            spot::twa::prop_set::all(), true);

                    cout << "SOG translated to SPOT succeeded.." << endl;
                    cout << "Want to save the graph in a dot file ?";
                    cin >> c;
                    if (c == 'y') {
                        fstream file;
                        string st(argv[3]);
                        st += ".dot";
                        file.open(st.c_str(), fstream::out);
                        spot::print_dot(file, k, "ka");
                        file.close();
                    }
                    if (auto run = k->intersecting_run(af)) {
                        run->highlight(5);
                        fstream file;
                        file.open("violated.dot", fstream::out);
                        cout << "Property is violated" << endl;
                        cout << "Check the dot file to get a violation run" << endl;
                        spot::print_dot(file, k, ".kA");
                        file.close();
                    } else
                        std::cout << "formula is verified\n";
                }

            }
        }


    }

    if (n_tasks > 1) {
        if (nb_th > 1) {
            if (task_id == 0)
                cout << "**************Hybrid version**************** \n" << endl;
            if (strcmp(argv[1], "otf") && strcmp(argv[1], "otfPR")  ) {
                HybridSOG DR(Rnewnet);
                LDDGraph g(&DR);
                if (task_id == 0)
                    cout << "Building the Distributed SOG by " << n_tasks << " processes..." << endl;
                DR.computeDSOG(g);
            } else {
                n_tasks--;
                if (task_id == n_tasks) {
                    cout << "Model checking on the fly..." << endl;
                    cout << " One process will perform Model checking" << endl;
                    cout << n_tasks << " process will build the Distributed SOG" << endl;
                }
                MPI_Comm gprocess;
                MPI_Comm_split(MPI_COMM_WORLD, task_id == n_tasks ? 0 : 1, task_id, &gprocess);
                //cout<<" Task id "<<task_id<<"/"<<n_tasks<<endl;
                if (task_id != n_tasks) {
                    cout << "N task :" << n_tasks << endl;
                    CommonSOG* DR;
                    if (strcmp(argv[1], "otf")==0) DR= new MCHybridSOG(Rnewnet, gprocess, false);
                    else {
                        cout<<"Progressive construction..."<<endl;
                        DR= new MCHybridSOGReq(Rnewnet, gprocess, false);
                    }
                    LDDGraph g(DR);
                    DR->computeDSOG(g);
                } else {
                    cout << "************************************" << endl;
                    cout << "On the fly Model checker by process " << task_id << endl;
                    auto d = spot::make_bdd_dict();
                    spot::twa_graph_ptr af = spot::translator(d).run(not_f);
                    auto k = std::make_shared<HybridKripke>(d, Rnewnet.getListTransitionAP(), Rnewnet.getListPlaceAP(), Rnewnet);
                    if (strcmp(algorithm, "")) {
                        std::shared_ptr < spot::twa_product > product = make_shared<spot::twa_product>(af, k);
                        const char *err;
                        spot::emptiness_check_instantiator_ptr echeck_inst = spot::make_emptiness_check_instantiator(algorithm, &err);

                        if (!echeck_inst) {
                            cerr << "Failed to parse argument near `" << err << "'" << endl;
                            cerr << "Spot unknown emptiness algorithm" << endl;
                            exit(2);
                        }
                        else
                            cout<<"Spot emptiness check algorithm : "<<algorithm<<endl;
                        auto startTime = std::chrono::high_resolution_clock::now();
                        spot::emptiness_check_ptr echptr = echeck_inst->instantiate(product);
                        bool res = (echptr->check() == 0);
                        auto finalTime = std::chrono::high_resolution_clock::now();
                        displayTime(startTime, finalTime);
                        displayCheckResult(res);
                    } else {
                        auto startTime = std::chrono::steady_clock::now();
                        bool res = (k->intersecting_run(af) == 0);
                        auto finalTime = std::chrono::steady_clock::now();
                        displayTime(startTime, finalTime);
                        displayCheckResult(res);
                    }
                }
            }
        }
    }
    MPI_Finalize();
    exit(0);
	return (EXIT_SUCCESS);
}
