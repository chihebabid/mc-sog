#include <ctime>
#include <chrono>
#include <iostream>
#include <string>
#include <fstream>
#include <mpi.h>
#include <CLI11.hpp>
#include "LDDGraph.h"
#include <spot/kripke/kripke.hh>
#include "MCMultiCore/ModelCheckerCPPThread.h"
#include "MCMultiCore/ModelCheckerTh.h"
#include "MCMultiCore/ModelCheckThReq.h"
#include "MCMultiCore/MCCPPThPor.h"
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
#include "Hybrid/MCHybridReqPOR/MCHybridSOGReqPOR.h"
#include "Hybrid/MCHybridPOR/MCHybridSOGPOR.h"
#include "algorithm/CNDFS.h"
#include <typeinfo>
#include <atomic>
#include <thread>
#include <vector>
using namespace std;

int n_tasks, task_id;
unsigned int nb_th = 1; // used by the distributed model checker

struct Formula
{
    spot::formula f;
    set<string> propositions;
};

/**
 * Save a graph in a dot file
 *
 * @param graph Graph to be saved
 * @param filename Output filename
 * @param options Options to customized the print
 */
void saveGraph(spot::twa_graph_ptr graph, const string &filename, const char *options = nullptr)
{
    fstream file;
    file.open(filename.c_str(), fstream::out);
    spot::print_dot(file, graph, options);
    file.close();
}

/**
 * Parse a file  containing a LTL formula and return its negation
 *
 * @param fileName Path to LTL formula
 * @return Formula
 */
Formula negateFormula(const string &fileName)
{
    string input;
    set<string> transitionSet;
    ifstream file(fileName);

    cout << "______________________________________" << endl;
    cout << "Fetching formula ... " << endl;

    if (!file.is_open())
    {
        cout << "Cannot open formula file" << endl;
        exit(0);
    }

    getline(file, input);
    cout << "        Loaded formula: " << input << endl;
    file.close();

    spot::parsed_formula pf = spot::parse_infix_psl(input);
    if (pf.format_errors(std::cerr))
        exit(0);

    spot::formula fo = pf.f;
    if (!fo.is_ltl_formula())
    {
        std::cerr << "Only LTL formulas are supported.\n";
        exit(0);
    }

    spot::atomic_prop_set *p_list = spot::atomic_prop_collect(fo, nullptr);
    for (const auto &i : *p_list)
    {
        transitionSet.insert(i.ap_name());
    }
    cout << "Formula in SPIN format: ";
    print_spin_ltl(std::cout, fo) << endl;

    cout << "Building negation of the formula ... ";
    spot::formula not_f = spot::formula::Not(pf.f);
    cout << "done\n"
         << endl;

    return {not_f, transitionSet};
}

/**
 * Convert a formula inton an automaton
 *
 * @param f  Formula to be converted
 * @param bdd  BDD structure
 * @param save_dot Save the automaton of the negated formula in a dot file
 * @return spot::twa_graph_ptr
 */
spot::twa_graph_ptr formula2Automaton(const spot::formula &f, spot::bdd_dict_ptr bdd, bool save_dot = false)
{
    cout << "\nBuilding automata for not(formula)\n";
    spot::twa_graph_ptr af = spot::translator(bdd).run(f);
    cout << "Formula automata built." << endl;

    // save the generated automaton in a dot file
    if (save_dot)
    {
        saveGraph(af, "negated_formula.dot");
    }

    return af;
}

/**
 * Print the result of the verification
 *
 * @param res flag indicating if the property is true or false
 */
void displayCheckResult(bool res)
{
    if (res)
        cout << "\nProperty is verified..." << endl;
    else
        cout << "\nProperty is violated..." << endl;
}

/**
 * Display the elapsed time between two times
 *
 * @param startTime initial time
 * @param finalTime final time
 */
void displayTime(auto startTime, auto finalTime)
{
    cout << "Verification duration : " << std::chrono::duration_cast<std::chrono::milliseconds>(finalTime - startTime).count() << " milliseconds\n";
}

/**
 * Print information of the execution
 *
 * @param net Net to be verified
 * @param formula  Formula to be verified
 * @param algorithm Emptiness check algorithm to be used
 * @param nb_th  Number of threads of the execution
 */
void displayToolInformation(const string &net, const string &formula, const string &algorithm, int nb_th)
{
    // computes the current year
    time_t t = std::time(nullptr);
    tm *const pTInfo = std::localtime(&t);
    int year = 1900 + pTInfo->tm_year;

    cout << "PMC-SOG: Parallel Model Checking tool based on Symbolic Observation Graphs" << endl;
    cout << "Version " << PMCSOG_VERSION_MAJOR << "." << PMCSOG_VERSION_MINOR << "." << PMCSOG_VERSION_PATCH << endl;
    cout << "(c) 2018 - " << year << endl;
    cout << endl;
    cout << "          Net file: " << net << endl;
    cout << "      Formula file: " << formula << endl;
    cout << "Checking algorithm: " << algorithm << endl;
    cout << "         # threads: " << nb_th << endl;
}

/**
 * Get the multi-core model-checker
 *
 * @param net Net to be verified
 * @param nb_th Number of threads
 * @param thread_library Thread library
 * @param progressive Flag for a progressive construction of the SOG
 * @param por Flag for partial order reduction
 * @return ModelCheckBaseMT*
 */
ModelCheckBaseMT *getMultiCoreMC(NewNet &net, int nb_th, const string &thread_library, bool progressive, bool por)
{
    ModelCheckBaseMT *mcl = nullptr;

    // select the right model-checking algorithm
    if (thread_library == "posix")
    {        if (progressive)

        {
            cout << "Multi-threaded algorithm (progressive) based on PThread!" << endl;
            mcl = new ModelCheckThReq(net, nb_th);
        }
        else
        {
            cout << "Multi-threaded algorithm based on Pthread library!" << endl;
            mcl = new ModelCheckerTh(net, nb_th);
        }
    }
    else if (thread_library == "c++")
    {
        if (por)
        {
            cout << "Multi-threaded algorithm based on C++ Thread library with POR!" << endl;
            mcl = new MCCPPThPor(net, nb_th);
        }
        else
        {
            cout << "Multi-threaded algorithm based on C++ Thread library!" << endl;
            mcl = new ModelCheckerCPPThread(net, nb_th);
        }
    }
    else
    {
        cout << thread_library << " is not a valid thread library for the multi-core version" << endl;
        exit(-1);
    }

    return mcl;
}

/**
 * Get the multi-core construction option of the SOG
 *
 * @param thread_library Thread library
 * @param canonization Flag to apply canonization
 * @param por Flag for partial order reduction
 * @return int
 */
int getMultiCoreBuildOption(const string &thread_library, bool canonization, bool por)
{
    if (thread_library == "posix")
    {
        if (canonization)
        {
            cout << "Canonized construction with pthread library." << endl;
            return 1;
        }
        else
        {
            if (por)
            {
                cout << "Partial Order Reduction with pthread library." << endl;
                return 2;
            }
            else
            {
                cout << "Construction with pthread library." << endl;
                return 0;
            }
        }
    }
    else
    {
        cout << thread_library << " is not a valid threads library for the multi-core version" << endl;
        exit(-1);
    }
}

/**
 * Get the Hybrid model-checker
 *
 * @param net Net to be verified
 * @param method  Hybrid implementation
 * @param progressive Flag for a progressive construction of the SOG
 * @param por Flag for partial order reduction
 * @param communicator MPI communicator
 * @return CommonSOG*
 */
CommonSOG *getHybridMC(NewNet &net, const string &thread_library, bool progressive, bool por, MPI_Comm &communicator)
{

    CommonSOG *DR;

    if (thread_library == "c++")
    {
        if (progressive)
        {
            if (por)
            {
                cout << " Progressive construction with POR..." << endl;
                DR = new MCHybridSOGReqPOR(net, communicator, false);
            }
            else
            {
                cout << " Progressive construction..." << endl;
                DR = new MCHybridSOGReq(net, communicator, false);
            }
        }
        else
        {
            if (por)
            {
                cout << " Normal construction with POR" << endl;
                DR = new MCHybridSOGPOR(net, communicator, false);
            }
            else
            {
                cout << " Normal construction..." << endl;
                DR = new MCHybridSOG(net, communicator, false);
            }
        }
    }
    else
    {
        cout << thread_library << " is not a valid thread library for the hybrid version" << endl;
        exit(-1);
    }

    return DR;
}

/**
 * Run the on-the-fly model-checking algorithm
 *
 * @param algorithm Sequential emptiness-check algorithm
 * @param k Kripke structure of the SOG
 * @param af Automata  of the negated formula
 * @return bool
 */
bool runOnTheFlyMC(const string &algorithm, auto k, spot::twa_graph_ptr af)
{
    bool res = false;
    std::chrono::steady_clock::time_point startTime, finalTime;

    cout << "\nPerforming on the fly Model-Checking..." << endl;

    if (algorithm != "")
    {
        // set the synchronized product of the SOG and the negated formula
        shared_ptr<spot::twa_product> product = make_shared<spot::twa_product>(k, af);


        // set the emptiness check
        const char *err;
        spot::emptiness_check_instantiator_ptr echeck_inst = spot::make_emptiness_check_instantiator(algorithm.c_str(), &err);

        if (!echeck_inst)
        {
            cerr << "Failed to parse argument near '" << err << "'" << endl;
            cerr << "Spot unknown emptiness algorithm" << endl;
            exit(2);
        }
        else
            cout << "Spot emptiness check algorithm: " << algorithm << endl;

        // computes the on-the-fly emptiness check of the synchronized product
        startTime = std::chrono::steady_clock::now();
        spot::emptiness_check_ptr echptr = echeck_inst->instantiate(product);
        res = (echptr->check() == nullptr);
        finalTime = std::chrono::steady_clock::now();
    }
    else
    {
        // computes the intersection between the SOG and the negated formula
        startTime = std::chrono::steady_clock::now();
        res = (k->intersecting_run(af) == nullptr);
        finalTime = std::chrono::steady_clock::now();
    }

    // display results
    displayTime(startTime, finalTime);
    displayCheckResult(res);

    return res;
}

/**
 * Run the on-the-fly parallel model-checking algorithm
 *
 * @param algorithm Parallel emptiness-check algorithm
 *
 * TODO: implement this
 */
//bool runOnTheFlyParallelMC(const string &algorithm) {}

/******************************************************************************
 * Main function
 ******************************************************************************/
int main(int argc, char **argv)
{
    CLI::App app{"PMC-SOG : Parallel Model Checking tool based on Symbolic Observation Graphs"};

    app.add_option("--n", nb_th, "Number of threads to be created (default: 1)")
        ->type_name("Integer")
        ->check(CLI::PositiveNumber);

    string net = "";
    app.add_option("--net", net, "Petri net file")
        ->type_name("Path")
        ->required()
        ->check(CLI::ExistingFile);

    string formula = "";
    app.add_option("--ltl", formula, "LTL property file")
        ->type_name("Path")
        ->required()
        ->check(CLI::ExistingFile);

    string thread_library = "c++";
    app.add_option("--thread", thread_library, "Thread library (default: c++)")->check(CLI::IsMember({"posix", "c++"}));

    bool por{false};
    app.add_flag("--por,!--no-por", por, "Apply partial order reductions (default: false)");

    bool explicit_mc{false};
    CLI::Option *exp_opt = app.add_flag("--explicit", explicit_mc, "Run explicit model-checking")->group("Explicit MC");

    // progressive construction can be used only when using on-the-fly model-checking
    bool progressive{false};
    app.add_flag("--progressive,!--no-progressive", progressive, "Use a progressive construction of the SOG (default: false)")->excludes(exp_opt)->group("On-the-fly MC");

    // emptiness check algorithm is only needed when using on-the-fly model-checking
    string algorithm = "";
    app.add_option("--emptiness-check", algorithm, "Spot emptiness-check algorithm")->excludes(exp_opt)->group("On-the-fly MC");

    // lace is only available when using explicit multi-core model-checking
    bool uselace{false};
    app.add_flag("--lace,!--no-lace", uselace, "Use the work-stealing framework Lace. Available only in multi-core (default: false)")->needs(exp_opt)->group("Explicit MC");

    // canonization is only available in the explicit multi-core version. In the hybrid is always true
    bool canonization{false};
    app.add_flag("--canonization,!--no-canonization", canonization, "Apply canonization to the SOG. Available only in multi-core (default: false)")->needs(exp_opt)->group("Explicit MC");

    // build only the SOG is possible only in explicit model-checking
    bool only_sog{false};
    app.add_flag("--only-sog", only_sog, "Only builds the SOG. Available only in multi-core")->needs(exp_opt)->group("Explicit MC");

    bool dot_sog{false};
    app.add_flag("--dot-sog", dot_sog, "Save the SOG in a dot file")->needs(exp_opt)->group("Print");

    bool dot_formula{false};
    app.add_flag("--dot-formula", dot_formula, "Save the automata of the negated formula in a dot file")->group("Print");

    bool counter_example{false};
    app.add_flag("--counter-example", counter_example, "Save the counter example in a dot file")->needs(exp_opt)->group("Print");

    CLI11_PARSE(app, argc, argv);

    // MPI arguments
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &n_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &task_id);

    // Display information of the tool
    if (!task_id)
    {
        displayToolInformation(net, formula, algorithm, nb_th);
    }

    // Check the number of processes
    if (n_tasks <= 0)
    {
        cout << "Number of tasks <= 0 " << endl;
        exit(0);
    }

    // Check the number of threads
    if (nb_th <= 0)
    {
        cout << "Number of thread <= 0 " << endl;
        exit(0);
    }

    // hybrid version requires two or more threads
    if (n_tasks > 1 && nb_th < 2)
    {
        if (task_id == 0)
            cout << "The hybrid version requires more than 1 threads\n"
                 << endl;
        exit(0);
    }

    // Get the negation of the LTL formula
    Formula negate_formula = negateFormula(formula);
    NewNet Rnewnet(net.c_str(), negate_formula.propositions);
    cout << endl;

    /**************************************************************************
     * Multi-core version
     **************************************************************************/
     //n_tasks=1;
    if (n_tasks == 1)
    {
        cout<< "Hello Multi-core version" <<endl;
        // On-the-fly multi-core model checking
        if (!explicit_mc)
        {
            // get the corresponding multi-core model-checker
           // exit(0);
            ModelCheckBaseMT *mcl = getMultiCoreMC(Rnewnet, nb_th, thread_library, progressive, por);

            // build automata of the negation of the formula
            auto d = spot::make_bdd_dict();
            spot::twa_graph_ptr af = formula2Automaton(negate_formula.f, d, dot_formula);

                       // create the SOG
            mcl->loadNet();

            // run on the fly parallel model-checking
            // TODO: Implement here Ghofrane's algorithms
            if (algorithm == "UFSCC" || algorithm == "CNDFS")
            {

                std::cout<<"I'm here"<<std::endl;

                CNDFS cndfs(mcl,af,3);

                // You have now to call a non static method of object cndfs that will create threads and execute your dfs algorithm
                // Your static method should be defined as private and called by a non static method
                // try else you send me a request tomorrw if you have pbs
                //CNDFS cndfs(*mcl, aa);
                //cndfs.DfsBlue(*mcl, aa);

               // n.spawnThreads(2,*mcl,aa);
                return(0);

            }
            else // run on the fly sequential model-checking
            {
                auto k = std::make_shared<SogKripkeTh>(d, mcl, Rnewnet.getListTransitionAP(), Rnewnet.getListPlaceAP());
                //runOnTheFlyMC(algorithm, k, af);
            }

            // stop model checker process
            mcl->finish();

            // print SOG information
            mcl->getGraph()->printCompleteInformation();
            delete mcl;
        }
        else // build only SOG
        {
            threadSOG DR(Rnewnet, nb_th, uselace);
            LDDGraph g(&DR);

            // Sequential  construction of the SOG
            // TODO: There is no way to run Model-checking when the SOG is built sequentially?
            if (nb_th == 1)
            {
                cout << "\n****************** Sequential version ******************* \n"<< endl;

                // build sequential SOG
                DR.computeSeqSOG(g);

                // print SOG information
                g.printCompleteInformation();
            }
            else // Multi-thread construction of the SOG
            {
                cout << "\n******************* Multi-thread version ****************** \n"
                     << endl;
                cout << "# of threads to be created: " << nb_th << endl;

                // select a method to construct the SOG
                int constructionOption = getMultiCoreBuildOption(thread_library, canonization, por);

                // build the distributed SOG
                DR.computeDSOG(g, constructionOption);

                // print SOG information
                g.printCompleteInformation();
                // run model checking process
                if (!only_sog)
                {
                    auto d = spot::make_bdd_dict();

                    // build the negation of the formula
                    spot::twa_graph_ptr af = formula2Automaton(negate_formula.f, d, dot_formula);

                    // build a twa graph of the SOG
                    cout << "\nTranslating SOG to SPOT ..." << endl;
                    spot::twa_graph_ptr k = spot::make_twa_graph(
                        std::make_shared<SogKripke>(d, DR.getGraph(), Rnewnet.getListTransitionAP(), Rnewnet.getListPlaceAP()),
                        spot::twa::prop_set::all(), true);
                    if (dot_sog)
                    {
                        saveGraph(af, "SOG.dot", "ka");
                    }
                    cout << endl;

                    // computes the explicit synchronization product
                    auto run = k->intersecting_run(af);
                    displayCheckResult(run == nullptr);

                    // display a counter-example if it exists
                    if (run && counter_example)
                    {
                        run->highlight(5);
                        saveGraph(k, "counter-example.dot", ".kA");
                        cout << "Check the dot file to get a counter-example" << endl;
                    }
                }
            }
        }
    }
    /**************************************************************************
     * Hybrid version
     **************************************************************************/
    else
    {
        cout << "hello hybrid version" <<endl;
        if (task_id == 0)
            cout << "\n************** Hybrid version ****************\n"
                 << endl;

        // On-the-fly hybrid model checking
        if (!explicit_mc)
        {
            n_tasks--;

            // ID of the task performing the model checking
            int mc_task = n_tasks;

            // creates model-checking process
            MPI_Comm gprocess;
            MPI_Comm_split(MPI_COMM_WORLD, task_id == mc_task ? 0 : 1, task_id, &gprocess);

            // computes the SOG in processes different to the model-checker one
            if (task_id != mc_task)
            {
                CommonSOG *DR = getHybridMC(Rnewnet, thread_library, progressive, por, gprocess);
                cout << "# tasks: " << n_tasks << endl;

                // compute the distributed SOG
                LDDGraph g(DR);
                DR->computeDSOG(g);
            }
            else // computes the on-the-fly model-checking in the mc process
            {
                cout << "Model checking on the fly..." << endl;
                cout << " 1 process will perform Model checking: process " << task_id << endl;
                cout << " " << n_tasks << " process(es) will build the Distributed SOG" << endl;

                // build the negation of the formula
                auto d = spot::make_bdd_dict();
                spot::twa_graph_ptr af = formula2Automaton(negate_formula.f, d, dot_formula);

                // build the SOG
                auto k = std::make_shared<HybridKripke>(d, Rnewnet.getListTransitionAP(), Rnewnet.getListPlaceAP(), Rnewnet);

               // runOnTheFlyMC(algorithm, k, af);
            }
        }
        else // build only the SOG
        {
            HybridSOG DR(Rnewnet);
            LDDGraph g(&DR);
            cout << endl;

            if (task_id == 0)
                cout << "Building the Distributed SOG by " << n_tasks << " processes..." << endl;

            // computes the distributed SOG
            DR.computeDSOG(g);
        }
    }

    MPI_Finalize();
    return (EXIT_SUCCESS);
}