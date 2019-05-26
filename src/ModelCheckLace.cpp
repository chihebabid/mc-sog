#include "ModelCheckLace.h"

#include "sylvan.h"
#include "sylvan_seq.h"
#include <sylvan_sog.h>
#include <sylvan_int.h>

using namespace sylvan;
ModelCheckLace::ModelCheckLace(const NewNet &R, int BOUND)
{
 m_nb_thread=nbThread;
    if (uselace)  {
    lace_init(m_nb_thread, 10000000);
    }
    else lace_init(1, 10000000);
    lace_startup(0, NULL, NULL);

    // Simple Sylvan initialization, also initialize MDD support
    //sylvan_set_sizes(1LL<<27, 1LL<<27, 1LL<<20, 1LL<<22);
    sylvan_set_limits(2LL<<30, 16, 3);
    //sylvan_set_sizes(1LL<<30, 2LL<<20, 1LL<<18, 1LL<<20);
    //sylvan_init_bdd(1);
    sylvan_init_package();
    sylvan_init_ldd();
    LACE_ME;
    /*sylvan_gc_hook_pregc(TASK(gc_start));
    sylvan_gc_hook_postgc(TASK(gc_end));
    sylvan_gc_enable();*/
       m_net=R;

    m_init=init;
    int  i;
    vector<Place>::const_iterator it_places;


    init_gc_seq();


    //_______________
    transitions=R.transitions;
    m_observable=R.Observable;
    m_place_proposition=R.m_formula_place;
    m_nonObservable=R.NonObservable;

    m_transitionName=R.transitionName;
    m_placeName=R.m_placePosName;

    cout<<"Toutes les Transitions:"<<endl;
    map<string,int>::iterator it2=m_transitionName.begin();
    for (;it2!=m_transitionName.end();it2++) {
        cout<<(*it2).first<<" : "<<(*it2).second<<endl;}



    cout<<"Transitions observables :"<<endl;
    Set::iterator it=m_observable.begin();
    for (;it!=m_observable.end();it++) {cout<<*it<<"  ";}
    cout<<endl;
    InterfaceTrans=R.InterfaceTrans;
    m_nbPlaces=R.places.size();
    cout<<"Nombre de places : "<<m_nbPlaces<<endl;
    cout<<"Derniere place : "<<R.places[m_nbPlaces-1].name<<endl;

    uint32_t * liste_marques=new uint32_t[R.places.size()];
    for(i=0,it_places=R.places.begin(); it_places!=R.places.end(); i++,it_places++)
    {
        liste_marques[i] =it_places->marking;
    }

    M0=lddmc_cube(liste_marques,R.places.size());





    uint32_t *prec = new uint32_t[m_nbPlaces];
    uint32_t *postc= new uint32_t [m_nbPlaces];
    // Transition relation
    for(vector<Transition>::const_iterator t=R.transitions.begin();
            t!=R.transitions.end(); t++)
    {
        // Initialisation
        for(i=0; i<m_nbPlaces; i++)
        {
            prec[i]=0;
            postc[i]=0;
        }
        // Calculer les places adjacentes a la transition t
        set<int> adjacentPlace;
        for(vector< pair<int,int> >::const_iterator it=t->pre.begin(); it!=t->pre.end(); it++)
        {
            adjacentPlace.insert(it->first);
            prec[it->first] = prec[it->first] + it->second;
            //printf("It first %d \n",it->first);
            //printf("In prec %d \n",prec[it->first]);
        }
        // arcs post
        for(vector< pair<int,int> >::const_iterator it=t->post.begin(); it!=t->post.end(); it++)
        {
            adjacentPlace.insert(it->first);
            postc[it->first] = postc[it->first] + it->second;

        }
        for(set<int>::const_iterator it=adjacentPlace.begin(); it!=adjacentPlace.end(); it++)
        {
            MDD Precond=lddmc_true;
            Precond = Precond & ((*it) >= prec[*it]);
        }

        MDD _minus=lddmc_cube(prec,m_nbPlaces);
        ldd_refs_push(_minus);
        MDD _plus=lddmc_cube(postc,m_nbPlaces);
        ldd_refs_push(_plus);
        m_tb_relation.push_back(TransSylvan(_minus,_plus));
    }

   // sylvan_gc_seq();


    delete [] prec;
    delete [] postc;
}
