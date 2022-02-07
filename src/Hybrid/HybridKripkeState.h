#ifndef HYBRIDKRIPKESTATETH_H_INCLUDED
#define HYBRIDKRIPKESTATETH_H_INCLUDED

#include <mpi.h>
#include "LDDState.h"
#include "ModelCheckBaseMT.h"
#include "SylvanWrapper.h"

constexpr auto TAG_ASK_STATE = 9;
constexpr auto TAG_ACK_STATE = 10;
constexpr auto TAG_ASK_SUCC = 4;
constexpr auto TAG_ACK_SUCC = 11;

typedef struct {
    char id[16];
    int16_t transition;
    uint16_t pcontainer;
    bool _virtual;
} succ_t;

class HybridKripkeState : public spot::state {
public:
    //static ModelCheckBaseMT * m_builder;
    HybridKripkeState(succ_t &e) : m_container(e.pcontainer) {

        if (e._virtual) { //div or deadlock
            /*if (e.id[0]=='v') { // div
                //cout<<"div created..."<<endl;
                m_hashid=0xFFFFFFFFFFFFFFFE;
                m_id[0]='v';
                succ_t elt;
                elt.id[0]='v';                   
                elt.transition=-1;
                elt._virtual=true;
                m_succ.push_back(elt);
            }*/
            /*else {*/
            m_id[0] = 'd';
            m_hashid = 0xFFFFFFFFFFFFFFFF; // deadlock
            /* }*/
        } else {
            memcpy(m_id, e.id, 16);
            MPI_Send(m_id, 16, MPI_BYTE, e.pcontainer, TAG_ASK_STATE, MPI_COMM_WORLD);
            MPI_Status status; //int nbytes;
            MPI_Probe(MPI_ANY_SOURCE, TAG_ACK_STATE, MPI_COMM_WORLD, &status);
            uint32_t nbytes;
            MPI_Get_count(&status, MPI_BYTE, &nbytes);
            // Receive hash id =idprocess | position
            char message[nbytes];

            MPI_Recv(message, nbytes, MPI_BYTE, MPI_ANY_SOURCE, TAG_ACK_STATE, MPI_COMM_WORLD, &status);

            memcpy(&m_hashid, message, 8);
            m_hashid = m_hashid | (size_t(m_container) << 56);

            // Determine Place propositions
            uint16_t n_mp;
            memcpy(&n_mp, message + 8, 2);

            //cout<<" Marked places :"<<n_mp<<endl;
            uint32_t indice = 10;
            for (uint16_t i = 0; i < n_mp; ++i) {
                uint16_t val;
                memcpy(&val, message + indice, 2);
                m_marked_places.emplace_front(val);
                indice += 2;
            }
            uint16_t n_up;
            memcpy(&n_up, message + indice, 2);
            //cout<<" Unmarked places :"<<n_mp<<endl;
            indice += 2;
            for (uint16_t i = 0; i < n_up; ++i) {
                uint16_t val;
                memcpy(&val, message + indice, 2);
                m_unmarked_places.emplace_front(val);
                indice += 2;
            }
            // Get div & deadlock
            uint8_t divblock;
            memcpy(&divblock, message + indice, 1);
            m_div = divblock & 1;
            m_deadlock = (divblock >> 1) & 1;
            indice++; // Added for stats
            memcpy(&m_size, message + indice, 8); // Added for stats

            // Get successors
            MPI_Send(m_id, 16, MPI_BYTE, m_container, TAG_ASK_SUCC, MPI_COMM_WORLD);

            // Receive list of successors

            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            MPI_Get_count(&status, MPI_BYTE, &nbytes);
            char inmsg[nbytes + 1];
            MPI_Recv(inmsg, nbytes, MPI_BYTE, status.MPI_SOURCE, TAG_ACK_SUCC, MPI_COMM_WORLD, &status);
            uint32_t nb_succ;
            memcpy(&nb_succ, inmsg, 4);
            indice = 4;
            m_succ.reserve(nb_succ + 2);
            if (m_deadlock) {
                succ_t el;
                el.id[0] = 'd';
                el.pcontainer = 0;
                el._virtual = true;
                el.transition = -1;
                m_succ.push_back(el);
            }
            if (m_div) {
                succ_t el = e;
                el._virtual = false;
                el.transition = -1;
                m_succ.push_back(el);
            }
            succ_t succ_elt;
            //printf("List of successors of %.16s\n",m_id);
            for (uint32_t i = 0; i < nb_succ; ++i) {
                //succ_elt=new succ_t;
                memcpy(succ_elt.id, inmsg + indice, 16);
                indice += 16;

                memcpy(&(succ_elt.pcontainer), inmsg + indice, 2);
                indice += 2;
                memcpy(&(succ_elt.transition), inmsg + indice, 2);
                indice += 2;
                succ_elt._virtual = false;
                m_succ.push_back(succ_elt);
            }


        }


    }

    // Constructor for cloning
    HybridKripkeState(unsigned char *id, uint16_t pcontainer, size_t hsh, bool ddiv, bool deadlock) : m_container(
            pcontainer), m_div(ddiv), m_deadlock(deadlock), m_hashid(hsh) {
        memcpy(m_id, id, 16);

        //cout<<__func__<<endl;

    }

    virtual ~HybridKripkeState();

    [[nodiscard]] HybridKripkeState *clone() const override {
        //cout<<__func__<<endl;
        return new HybridKripkeState(m_id, m_container, m_hashid, m_div, m_deadlock);
    }

    [[nodiscard]] size_t hash() const override {
        return m_hashid;
    }

    int compare(const spot::state *other) const override {

        auto o = static_cast<const HybridKripkeState *>(other);
        size_t oh = o->hash();
        size_t h = hash();
        //return (h!=oh);
        if (h < oh)
            return -1;
        else
            return h > oh;
    }

    char *getId() {
        return m_id;
    }

    uint16_t getContainerProcess() {
        return m_container;
    }

    list<uint16_t> *getMarkedPlaces() { return &m_marked_places; }

    list<uint16_t> *getUnmarkedPlaces() { return &m_unmarked_places; }

    vector<succ_t> *getListSucc() { return &m_succ; }

    uint64_t getSize() const { return m_size; } // Added for stats
protected:

private:
    char m_id[17];
    size_t m_hashid;
    bool m_div;
    bool m_deadlock;
    uint64_t m_size;
    uint16_t m_container;
    list<uint16_t> m_marked_places;
    list<uint16_t> m_unmarked_places;
    vector<succ_t> m_succ;
};


#endif
