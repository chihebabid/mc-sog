#include "ModelCheckerTh.h"

#include "sylvan.h"
#include "sylvan_seq.h"
#include <sylvan_sog.h>
#include <sylvan_int.h>

using namespace sylvan;

ModelCheckerTh::ModelCheckerTh(const NewNet &R, int nbThread) :
		ModelCheckBaseMT(R, nbThread) {
}
void ModelCheckerTh::preConfigure() {

	lace_init(1, 0);
	lace_startup(0, NULL, NULL);

	sylvan_set_limits(2LL << 30, 16, 3);
	sylvan_init_package();
	sylvan_init_ldd();

	int i;
	vector<Place>::const_iterator it_places;

	init_gc_seq();


	transitions = m_net->transitions;
	m_observable = m_net->Observable;
	m_place_proposition = m_net->m_formula_place;
	m_nonObservable = m_net->NonObservable;

	m_transitionName = m_net->transitionName;
	m_placeName = m_net->m_placePosName;

	InterfaceTrans = m_net->InterfaceTrans;

	cout << "Nombre de places : " << m_nbPlaces << endl;
	cout << "Derniere place : " << m_net->places[m_nbPlaces - 1].name << endl;

	uint32_t *liste_marques = new uint32_t[m_net->places.size()];
	for (i = 0, it_places = m_net->places.begin(); it_places != m_net->places.end(); i++, it_places++) {
		liste_marques[i] = it_places->marking;
	}

	m_initialMarking = lddmc_cube(liste_marques, m_net->places.size());

	uint32_t *prec = new uint32_t[m_nbPlaces];
	uint32_t *postc = new uint32_t[m_nbPlaces];
	// Transition relation
	for (vector<Transition>::const_iterator t = m_net->transitions.begin(); t != m_net->transitions.end(); t++) {
		// Initialisation
		for (i = 0; i < m_nbPlaces; i++) {
			prec[i] = 0;
			postc[i] = 0;
		}
		// Calculer les places adjacentes a la transition t
		set<int> adjacentPlace;
		for (vector<pair<int, int> >::const_iterator it = t->pre.begin(); it != t->pre.end(); it++) {
			adjacentPlace.insert(it->first);
			prec[it->first] = prec[it->first] + it->second;
			//printf("It first %d \n",it->first);
			//printf("In prec %d \n",prec[it->first]);
		}
		// arcs post
		for (vector<pair<int, int> >::const_iterator it = t->post.begin(); it != t->post.end(); it++) {
			adjacentPlace.insert(it->first);
			postc[it->first] = postc[it->first] + it->second;

		}
		for (set<int>::const_iterator it = adjacentPlace.begin(); it != adjacentPlace.end(); it++) {
			MDD Precond = lddmc_true;
			Precond = Precond & ((*it) >= prec[*it]);
		}

		MDD _minus = lddmc_cube(prec, m_nbPlaces);
		ldd_refs_push(_minus);
		MDD _plus = lddmc_cube(postc, m_nbPlaces);
		ldd_refs_push(_plus);
		m_tb_relation.push_back(TransSylvan(_minus, _plus));
	}
	delete[] prec;
	delete[] postc;
	ComputeTh_Succ();

}

LDDState* ModelCheckerTh::getInitialMetaState() {
	while (!m_finish_initial)
		;
	LDDState *initial_metastate = m_graph->getInitialState();
	if (!initial_metastate->isVisited()) {
		initial_metastate->setVisited();
		while (!initial_metastate->isCompletedSucc())
			;
	}
	return initial_metastate;
}

void ModelCheckerTh::buildSucc(LDDState *agregate) {
	if (!agregate->isVisited()) {
		agregate->setVisited();
		while (!agregate->isCompletedSucc())
			;
	}
}

void* ModelCheckerTh::Compute_successors() {
	int id_thread;
	pthread_mutex_lock(&m_mutex);
	id_thread = m_id_thread++;
	pthread_mutex_unlock(&m_mutex);
	Set fire;
	int min_charge = 0;
	if (id_thread == 0) {
		LDDState *c = new LDDState;

		MDD Complete_meta_state(Accessible_epsilon(m_initialMarking));
		ldd_refs_push(Complete_meta_state);
		MDD canonised_initial = Complete_meta_state;            //Canonize(Complete_meta_state,0);
		ldd_refs_push(canonised_initial);
		fire = firable_obs(Complete_meta_state);
		c->m_lddstate = canonised_initial;
		c->setDeadLock(Set_Bloc(Complete_meta_state));
		c->setDiv(Set_Div(Complete_meta_state));
		m_st[0].push(Pair(couple(c, Complete_meta_state), fire));
		m_graph->setInitialState(c);
		m_graph->insert(c);
		m_charge[0] = 1;
		m_finish_initial = true;
	}

	LDDState *reached_class;
	do {
		while (!m_st[id_thread].empty() && !m_finish) {
			m_terminaison[id_thread] = false;
			pthread_spin_lock(&m_spin_stack[id_thread]);
			Pair e = m_st[id_thread].top();
			//pthread_spin_lock(&m_accessible);
			m_st[id_thread].pop();
			pthread_spin_unlock(&m_spin_stack[id_thread]);
			m_charge[id_thread]--;
			while (!e.second.empty() && !m_finish) {
				int t = *e.second.begin();
				e.second.erase(t);

				if (id_thread) {
					pthread_mutex_lock(&m_supervise_gc_mutex);
					m_gc++;
					if (m_gc == 1) {
						pthread_mutex_lock(&m_gc_mutex);
					}
					pthread_mutex_unlock(&m_supervise_gc_mutex);
				}

				MDD Complete_meta_state = Accessible_epsilon(get_successor(e.first.second, t));

				ldd_refs_push(Complete_meta_state);
				MDD reduced_meta = Complete_meta_state;            //Canonize(Complete_meta_state,0);
				ldd_refs_push(reduced_meta);

				if (id_thread == 0) {
					pthread_mutex_lock(&m_gc_mutex);
					//     #ifdef DEBUG_GC
					//   displayMDDTableInfo();
					//   #endif // DEBUG_GC

					sylvan_gc_seq();
					//   #ifdef DEBUG_GC
					//   displayMDDTableInfo();
					//   #endif // DEBUG_GC
					pthread_mutex_unlock(&m_gc_mutex);
				}
				reached_class = new LDDState();
				reached_class->m_lddstate = reduced_meta;
				//reached_class->m_lddstate=reduced_meta;
				//nbnode=bdd_pathcount(reached_class->m_lddstate);

				//pthread_spin_lock(&m_accessible);
				pthread_mutex_lock(&m_graph_mutex);

				LDDState *pos = m_graph->find(reached_class);

				if (!pos) {
					//  cout<<"not found"<<endl;
					//reached_class->blocage=Set_Bloc(Complete_meta_state);
					//reached_class->boucle=Set_Div(Complete_meta_state);

					m_graph->addArc();
					m_graph->insert(reached_class);
					reached_class->setDiv(Set_Div(Complete_meta_state));
					reached_class->setDeadLock(Set_Bloc(Complete_meta_state));
					pthread_mutex_unlock(&m_graph_mutex);

					e.first.first->Successors.insert(e.first.first->Successors.begin(), LDDEdge(reached_class, t));
					reached_class->Predecessors.insert(reached_class->Predecessors.begin(), LDDEdge(e.first.first, t));
					//pthread_mutex_lock(&m_mutex);
					fire = firable_obs(Complete_meta_state);
					//if (max_succ<fire.size()) max_succ=fire.size();
					//pthread_mutex_unlock(&m_mutex);
					min_charge = minCharge();
					//m_min_charge=(m_min_charge+1) % m_nb_thread;
					pthread_spin_lock(&m_spin_stack[min_charge]);
					m_st[min_charge].push(Pair(couple(reached_class, Complete_meta_state), fire));
					pthread_spin_unlock(&m_spin_stack[min_charge]);
					m_charge[min_charge]++;
				} else {
					m_graph->addArc();
					pthread_mutex_unlock(&m_graph_mutex);
					e.first.first->Successors.insert(e.first.first->Successors.begin(), LDDEdge(pos, t));
					pos->Predecessors.insert(pos->Predecessors.begin(), LDDEdge(e.first.first, t));
					delete reached_class;

				}
				if (id_thread) {
					pthread_mutex_lock(&m_supervise_gc_mutex);
					m_gc--;
					if (m_gc == 0)
						pthread_mutex_unlock(&m_gc_mutex);
					pthread_mutex_unlock(&m_supervise_gc_mutex);
				}
			}
			e.first.first->setCompletedSucc();
		}
		m_terminaison[id_thread] = true;
	} while (isNotTerminated() && !m_finish);
	//cout<<"Thread id :"<<id_thread<<endl;
	pthread_barrier_wait(&m_barrier_builder);

}

void* ModelCheckerTh::threadHandler(void *context) {
	return ((ModelCheckerTh*) context)->Compute_successors();
}

void ModelCheckerTh::ComputeTh_Succ() {

	m_id_thread = 0;

	pthread_mutex_init(&m_mutex, NULL);
	pthread_mutex_init(&m_gc_mutex, NULL);
	pthread_mutex_init(&m_graph_mutex, NULL);
	pthread_mutex_init(&m_supervise_gc_mutex, NULL);
	/*   pthread_barrier_init(&m_barrier_threads, NULL, m_nb_thread+1);*/
	pthread_barrier_init(&m_barrier_builder, NULL, m_nb_thread + 1);

	for (int i = 0; i < m_nb_thread; i++) {
		pthread_spin_init(&m_spin_stack[i], NULL);
		m_charge[i] = 0;
		m_terminaison[i] = false;
	}

	for (int i = 0; i < m_nb_thread; i++) {
		int rc;
		if ((rc = pthread_create(&m_list_thread[i], NULL, threadHandler, this))) {
			cout << "error: pthread_create, rc: " << rc << endl;
		}
	}
}

ModelCheckerTh::~ModelCheckerTh() {
	m_finish = true;
	pthread_barrier_wait(&m_barrier_builder);
	for (int i = 0; i < m_nb_thread; i++) {
		pthread_join(m_list_thread[i], NULL);
	}

}

int ModelCheckerTh::minCharge() {
	int pos = 0;
	int min_charge = m_charge[0];
	for (int i = 1; i < m_nb_thread; i++) {
		if (m_charge[i] < min_charge) {
			min_charge = m_charge[i];
			pos = i;
		}
	}
	return pos;

}

bool ModelCheckerTh::isNotTerminated() {
	bool res = true;
	int i = 0;
	while (i < m_nb_thread && res) {
		res = m_terminaison[i];
		i++;
	}
	return !res;
}
