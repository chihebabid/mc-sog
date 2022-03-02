#include <ctime>
#include <chrono>
#include <iostream>
#include <string>
#include <fstream>
#include <mpi.h>
#include <CLI11.hpp>

#include "LDDGraph.h"
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

using namespace std;

int n_tasks, task_id;
unsigned int nb_th; // used by the distributed model checker

struct Formula
{
    spot::formula f;
    set<string> propositions;
};

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
 * @return spot::twa_graph_ptr
 */
spot::twa_graph_ptr formula2Automaton(const spot::formula &f, spot::bdd_dict_ptr bdd)
{
    cout << "\nBuilding automata for not(formula)\n";
    spot::twa_graph_ptr af = spot::translator(bdd).run(f);
    cout << "Formula automata built.\n"
         << endl;
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
        cout << "Property is verified..." << endl;
    else
        cout << "Property is violated..." << endl;
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
 * Checks if the options are valid to run the multi-core version
 *
 * @param nb_threads Number of threads
 * @param method  Method to build the threads
 * @return bool
 */
bool isValidMultiCoreMCOption(int nb_threads, const string &method)
{
    set<string> options = {"otfPR",
                           "otfC",
                           "otfP",
                           "otfCPOR"};

    return (nb_threads == 1 && (options.find(method) != options.end()));
}

/**
 * Checks if the options are valid to run the hybrid version
 *
 * @param nb_threads Number of threads
 * @param method  Method to build the distributed SOG
 * @return bool
 */
bool isValidHybridMCOption(int nb_threads, const string &method)
{
    set<string> options = {"otf",
                           "otfPR",
                           "otfPOR",
                           "otfPRPOR"};

    return (nb_threads > 1 && (options.find(method) != options.end()));
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
 * @param method Multi-core implementation
 * @param nb_th Number of threads
 * @return ModelCheckBaseMT*
 */
ModelCheckBaseMT *getMultiCoreMC(NewNet &net, const string &method, int nb_th)
{
    ModelCheckBaseMT *mcl = nullptr;

    // select the right model-checking algorithm
    if (method == "otfP")
    {
        cout << "Multi-threaded algorithm based on Pthread library!" << endl;
        mcl = new ModelCheckerTh(net, nb_th);
    }
    else if (method == "otfPR")
    {
        cout << "Multi-threaded algorithm (progressive) based on PThread!" << endl;
        mcl = new ModelCheckThReq(net, nb_th);
    }
    else if (method == "otfC")
    {
        cout << "Multi-threaded algorithm based on C++ Thread library!" << endl;
        mcl = new ModelCheckerCPPThread(net, nb_th);
    }
    else if (method == "otfCPOR")
    {
        cout << "Multi-threaded algorithm with POR!" << endl;
        mcl = new MCCPPThPor(net, nb_th);
    }
    else
    {
        cout << method << " is not valid for the multi-core version" << endl;
        exit(-1);
    }

    return mcl;
}

/**
 * Get the Hybrid model-checker
 *
 * @param net Net to be verified
 * @param method  Hybrid implementation
 * @param communicator MPI communicator
 * @return CommonSOG*
 */
CommonSOG *getHybridMC(NewNet &net, const string &method, MPI_Comm &communicator)
{

    CommonSOG *DR;
    if (method == "otf")
    {
        cout << " Normal construction..." << endl;
        DR = new MCHybridSOG(net, communicator, false);
    }
    else if (method == "otfPOR")
    {
        cout << " Normal construction with POR" << endl;
        DR = new MCHybridSOGPOR(net, communicator, false);
    }
    else if (method == "otfPR")
    {
        cout << " Progressive construction..." << endl;
        DR = new MCHybridSOGReq(net, communicator, false);
    }
    else if (method == "otfPRPOR")
    {
        cout << " Progressive construction with POR..." << endl;
        DR = new MCHybridSOGReqPOR(net, communicator, false);
    }
    else
    {
        cout << " " << method << " is not valid for the hybrid version" << endl;
        exit(0);
    }
    return DR;
}

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

void runOnTheFlyMC(const string &algorithm, auto k, spot::twa_graph_ptr af)
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
}

/******************************************************************************
 * Main function
 ******************************************************************************/
int main(int argc, char **argv)
{

    CLI::App app{"PMC-SOG : Parallel Model Checking tool based on Symbolic Observation Graphs"};

    app.add_option("--n", nb_th, "Number of threads/workers to be created (default: 1)")
        ->default_val(1)
        ->check(CLI::PositiveNumber);

    string net = "";
    app.add_option("--net", net, "Petri net file")
        ->required()
        ->check(CLI::ExistingFile);

    string formula = "";
    app.add_option("--ltl", formula, "LTL property file")
        ->required()
        ->check(CLI::ExistingFile);

    string method = "";
    app.add_option("--method", method, "Method to create threads or/and set if model-checking should be performed on the fly")
        ->required();

    string algorithm = "";
    app.add_option("--emptiness-check", algorithm, "Number of threads/workers to be created");

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
    if (n_tasks == 1)
    {
        // On-the-fly multi-core model checking
        if (isValidMultiCoreMCOption(n_tasks, method))
        {
            // get the corresponding multi-core model-checker
            ModelCheckBaseMT *mcl = getMultiCoreMC(Rnewnet, method, nb_th);

            // build automata of the negation of the formula
            auto d = spot::make_bdd_dict();
            spot::twa_graph_ptr af = formula2Automaton(negate_formula.f, d);

            // create the SOG
            mcl->loadNet();
            auto k = std::make_shared<SogKripkeTh>(d, mcl, Rnewnet.getListTransitionAP(), Rnewnet.getListPlaceAP());

            // run on the fly model-checking
            runOnTheFlyMC(algorithm, k, af);

            // stop model checker process
            mcl->finish();

            // print SOG information
            mcl->getGraph()->printCompleteInformation();
            delete mcl;
        }
        else // build only SOG
        {
            bool uselace = ((method == "lc") || (method == "l"));
            threadSOG DR(Rnewnet, nb_th, uselace);
            LDDGraph g(&DR);

            // Sequential  construction of the SOG
            // TODO: There is no way to run Model-checking when the SOG is built sequentially?
            if (nb_th == 1)
            {
                cout << "\n****************** Sequential version ******************* \n"
                     << endl;

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
                unsigned int constructionOption;
                if (method == "p")
                {
                    cout << "Construction with pthread library." << endl;
                    constructionOption = 0;
                }
                else if (method == "pc")
                {
                    cout << "Canonized construction with pthread library." << endl;
                    constructionOption = 1;
                }
                else
                {
                    cout << "Partial Order Reduction with pthread library." << endl;
                    constructionOption = 2;
                }

                // build the distributed SOG
                DR.computeDSOG(g, constructionOption);

                // print SOG information
                g.printCompleteInformation();

                cout << "\nPerform Model checking? ";
                char c;
                cin >> c;
                if (c == 'y')
                {
                    auto d = spot::make_bdd_dict();

                    // build the negation of the formula
                    spot::twa_graph_ptr af = formula2Automaton(negate_formula.f, d);
                    cout << "Want to save the automata of the formula in a dot file? ";
                    cin >> c;
                    if (c == 'y')
                    {
                        saveGraph(af, formula + ".dot");
                    }

                    // build a twa graph of the SOG
                    // TODO: Why do we translate the SOG into twa ?
                    cout << "\n\nTranslating SOG to SPOT ..." << endl;
                    spot::twa_graph_ptr k = spot::make_twa_graph(
                        std::make_shared<SogKripke>(d, DR.getGraph(), Rnewnet.getListTransitionAP(), Rnewnet.getListPlaceAP()),
                        spot::twa::prop_set::all(), true);
                    cout << "\nWant to save the SOG in a dot file? ";
                    cin >> c;
                    if (c == 'y')
                    {
                        saveGraph(af, net + ".dot", "ka");
                    }
                    cout << endl;

                    // computes the explicit synchronization product
                    auto run = k->intersecting_run(af);
                    displayCheckResult(run == nullptr);

                    // display a counter-example if it exists
                    if (run)
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
        if (task_id == 0)
            cout << "\n************** Hybrid version ****************\n"
                 << endl;

        // On-the-fly hybrid model checking
        if (isValidHybridMCOption(nb_th, method))
        {
            n_tasks--;

            // ID of the task performing the model checking
            int mc_task = n_tasks;

            // display information about model checking processes
            if (task_id == mc_task)
            {
                cout << "Model checking on the fly..." << endl;
                cout << " 1 process will perform Model checking" << endl;
                cout << " " << n_tasks << " process(es) will build the Distributed SOG" << endl;
            }

            // creates model-checking process
            MPI_Comm gprocess;
            MPI_Comm_split(MPI_COMM_WORLD, task_id == mc_task ? 0 : 1, task_id, &gprocess);

            // computes the SOG in processes different to the model-checker one
            if (task_id != mc_task)
            {
                CommonSOG *DR = getHybridMC(Rnewnet, method, gprocess);
                cout << "# tasks: " << n_tasks << endl;

                // compute the distributed SOG
                LDDGraph g(DR);
                DR->computeDSOG(g);
            }
            else // computes the on-the-fly model-checking in the mc process
            {
                cout << "On the fly Model checker by process " << task_id << endl;

                // build the negation of the formula
                auto d = spot::make_bdd_dict();
                spot::twa_graph_ptr af = formula2Automaton(negate_formula.f, d);

                // build the SOG
                auto k = std::make_shared<HybridKripke>(d, Rnewnet.getListTransitionAP(), Rnewnet.getListPlaceAP(), Rnewnet);

                runOnTheFlyMC(algorithm, k, af);
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