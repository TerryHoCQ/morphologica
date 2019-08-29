#ifndef _SHAPEANALYSIS_H_
#define _SHAPEANALYSIS_H_

#include <vector>
#include <list>
#include <set>
#include <limits>
#include <stdexcept>
#include "morph/Hex.h"
#include "morph/HexGrid.h"
#include "morph/DirichVtx.h"
#include "morph/MorphDbg.h"

using std::vector;
using std::list;
using std::set;
using std::numeric_limits;
using std::runtime_error;
using std::exception;

using morph::Hex;
using morph::HexGrid;
using morph::DirichVtx;

namespace morph {

    /*!
     * Rotational direction Rotn::Clock or Rotn::Anticlock
     */
    enum class Rotn {
        Unknown,
        Clock,
        Anticlock
    };

    /*!
     * A helper class, containing pattern analysis code to analyse
     * patterns within HexGrids.
     */
    template <class Flt>
    class ShapeAnalysis
    {
    public:
        /*!
         * Obtain the contours (as a vector of list<Hex>) in the scalar
         * fields f, where threshold is crossed.
         */
        static vector<list<Hex> > get_contours (HexGrid* hg,
                                                vector<vector<Flt> >& f,
                                                Flt threshold) {

            unsigned int nhex = hg->num();
            unsigned int N = f.size();

            vector<list<Hex> > rtn;
            // Initialise
            for (unsigned int li = 0; li < N; ++li) {
                list<Hex> lh;
                rtn.push_back (lh);
            }

            Flt maxf = -1e7;
            Flt minf = +1e7;
            for (auto h : hg->hexen) {
                if (h.onBoundary() == false) {
                    for (unsigned int i = 0; i<N; ++i) {
                        if (f[i][h.vi] > maxf) { maxf = f[i][h.vi]; }
                        if (f[i][h.vi] < minf) { minf = f[i][h.vi]; }
                    }
                }
            }
            Flt scalef = 1.0 / (maxf-minf);

            // Re-normalize
            vector<vector<Flt> > norm_f;
            norm_f.resize (N);
            for (unsigned int i=0; i<N; ++i) {
                norm_f[i].resize (nhex, 0.0);
            }

            for (unsigned int i = 0; i<N; ++i) {
                for (unsigned int h=0; h<nhex; h++) {
                    norm_f[i][h] = (f[i][h] - minf) * scalef;
                }
            }

            // Collate
            for (unsigned int i = 0; i<N; ++i) {
                for (auto h : hg->hexen) {
                    if (h.onBoundary() == false) {
                        if (norm_f[i][h.vi] > threshold) {
                            if ( (h.has_ne && norm_f[i][h.ne->vi] < threshold)
                                 || (h.has_nne && norm_f[i][h.nne->vi] < threshold)
                                 || (h.has_nnw && norm_f[i][h.nnw->vi] < threshold)
                                 || (h.has_nw && norm_f[i][h.nw->vi] < threshold)
                                 || (h.has_nsw && norm_f[i][h.nsw->vi] < threshold)
                                 || (h.has_nse && norm_f[i][h.nse->vi] < threshold) ) {
                                rtn[i].push_back (h);
                            }
                        }
                    } else { // h.onBoundary() is true
                        if (norm_f[i][h.vi] > threshold) {
                            rtn[i].push_back (h);
                        }
                    }
                }
            }

            return rtn;
        }

        /*!
         * Take a set of variables, @f, for the given HexGrid
         * @hg. Return a vector of Flts (again, based on the HexGrid
         * @hg) which marks each hex with the outer index of the @f
         * which has highest value in that hex, scaled and converted to a
         * float.
         */
        static vector<Flt>
        dirichlet_regions (HexGrid* hg, vector<vector<Flt> >& f) {
            unsigned int N = f.size();

            // Single variable to return
            vector<Flt> rtn (f[0].size(), 0.0);

            // Mark regions first.
            for (auto h : hg->hexen) {

                Flt maxf = -1e7;
                for (unsigned int i = 0; i<N; ++i) {
                    if (f[i][h.vi] > maxf) {
                        maxf = f[i][h.vi];
                        Flt fi = 0.0f;
                        fi = (Flt)i;
                        rtn[h.vi] = (fi/N);
                    }
                }
            }

            return rtn;
        }

        /*!
         * A method to test the hex give by @h, which must live on the
         * HexGrid pointed to by @hg, to see if it is a Dirichlet
         * vertex. If so, a vertex should be greated in @vertices.
         */
        static void
        vertex_test (HexGrid* hg, vector<Flt>& f,
                     list<Hex>::iterator h, list<DirichVtx<Flt> >& vertices) {

            // For each hex, examine its neighbours, counting number of different neighbours.
            set<Flt> n_ids;
            n_ids.insert (f[h->vi]);
            for (unsigned int ni = 0; ni < 6; ++ni) {
                if (h->has_neighbour(ni)) {
                    n_ids.insert (f[h->get_neighbour(ni)->vi]);
                }
            }

            if (h->boundaryHex == true && n_ids.size() == 2) { // 1. Test for boundary vertices

                // Here, I need to set a vertex where two hexes join and
                // we're on the boundary. This provides
                // information to set the angles to discover the
                // best center for each domain (see Honda 1983).

                for (int ni = 0; ni < 6; ++ni) { // ni==0 is neighbour east. 1 is neighbour NE, etc.

                    // If there's a neighbour in neighbour direction ni and that neighbour has different ID:
                    if (h->has_neighbour(ni) && f[h->get_neighbour(ni)->vi] != f[h->vi]) {

                        // Change this - examine which direction
                        // DOESN'T have a neighbour and that will
                        // determine which hex vertex is the
                        // domain vertex.

                        // The first non-identical ID
                        Flt f1 = f[h->get_neighbour(ni)->vi];
                        int nii = (ni+1)%6;
                        if (!h->has_neighbour(nii)) {
                            // Then vertex direction is "vertex direction ni"
                            vertices.push_back (
                                DirichVtx<Flt> (
                                    h->get_vertex_coord(ni),
                                    hg->getd(),
                                    f[h->vi],
                                    make_pair(-1.0f, f[h->get_neighbour(ni)->vi]),
                                    h)
                                );
                            break; // or set ni=6;

                        } else {

                            nii = ni>0 ? (ni-1) : 5;
                            if (!h->has_neighbour(nii)) {
                                // Then vertex direction is "vertex direction (ni-1) or 5", i.e. nii.
                                vertices.push_back (
                                    DirichVtx<Flt> (
                                        h->get_vertex_coord(nii),
                                        hg->getd(),
                                        f[h->vi],
                                        make_pair(f[h->get_neighbour(ni)->vi], -1.0f),
                                        h)
                                    );
                                break; // or set ni=6;
                            }
                        }
                    }
                }

            } else if (n_ids.size() > 2) { // 2. Test for internal vertices

                // >2 (i.e. 3) different types in self &
                // neighbouring hexes, so now work out which of
                // the Hex's vertices is the vertex of the domain.

                for (int ni = 0; ni < 6; ++ni) { // ni==0 is neighbour east. 1 is neighbour NE, etc.

                    // If there's a neighbour in direction ni and that neighbour has different ID:
                    if (h->has_neighbour(ni) && f[h->get_neighbour(ni)->vi] != f[h->vi]) {

                        // The first non-identical ID
                        Flt f1 = f[h->get_neighbour(ni)->vi];
                        int nii = (ni+1)%6;

                        if (h->has_neighbour(nii)
                            && f[h->get_neighbour(nii)->vi] != f[h->vi]
                            && f[h->get_neighbour(nii)->vi] != f1 // f1 already tested != f[h->vi]
                            ) {
                            // Then vertex is "vertex ni"
                            vertices.push_back (
                                DirichVtx<Flt>(
                                    h->get_vertex_coord(ni),
                                    hg->getd(),
                                    f[h->vi],
                                    make_pair(f[h->get_neighbour(nii)->vi], f[h->get_neighbour(ni)->vi]),
                                    h)
                                );
                            break;

                        } else {
                            nii = ni>0 ? (ni-1) : 5;
                            if (h->has_neighbour(nii)
                                && f[h->get_neighbour(nii)->vi] != f[h->vi]
                                && f[h->get_neighbour(nii)->vi] != f1 // f1 already tested != f[h->vi]
                                ) {
                                vertices.push_back (
                                    DirichVtx<Flt>(
                                        h->get_vertex_coord(nii),
                                        hg->getd(),
                                        f[h->vi],
                                        make_pair(f[h->get_neighbour(ni)->vi], f[h->get_neighbour(nii)->vi]),
                                        h)
                                    );
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Need to return some sort of reference to the vertex that we find at the end of the walk.
        static pair<Flt, Flt>
        walk_common (HexGrid* hg,
                     vector<Flt>& f,
                     DirichVtx<Flt>& v,
                     list<pair<Flt, Flt>>& path,
                     pair<Flt, Flt>& edgedoms,
                     Flt& next_neighb_dom) {

            DBG ("Called. edgedoms.first: "<< edgedoms.first << ", edgedoms.second: " << edgedoms.second);

            // Really, we only have coordinates to return.
            pair<Flt, Flt> next_one = {numeric_limits<Flt>::max(), numeric_limits<Flt>::max()};

            // Used later
            int i = 0;
            int j = 0;

            // Walk the edge, with hexit pointing to the hexes on the
            // edgedoms.first side. _Initially_, point hexit at the hex
            // that's on the inside of the domain for which v is a
            // Dirichlet vertex - v.hi.
            list<Hex>::iterator hexit = v.hi;
            // point hexit_neighb to the hexes on the edgedoms.second side
            list<Hex>::iterator hexit_neighb = v.hi;
            // The first hex, inside the domain.
            list<Hex>::iterator hexit_first = v.hi;
            // Temporary hex pointers
            list<Hex>::iterator hexit_next = v.hi;
            list<Hex>::iterator hexit_last = v.hi;

            // Set true when we find the partner vertex.
            bool partner_found = false;

            DBG ("Initial hex has f=" << f[hexit_first->vi] << ", " << hexit_first->outputRG() << " and initial neighbours:");
            for (int k = 0; k<6; ++k) {
                DBG (Hex::neighbour_pos(k) << ": " << f[hexit_first->get_neighbour (k)->vi]);
            }

            pair<Flt, Flt> v_init = v.v;

            // Test if the initial hex itself is on the side of the edge (as it will be when walking from one domain vertex
            // to the next domain vertex). This code is side-stepped when walking to a "B_i" neighbour vertex.
            if (f[hexit_first->vi] == edgedoms.first) {
                DBG ("Hex AT " << hexit_first->outputRG() << " has f=" << f[hexit_first->vi]);
                // In this case, I need to find out which of the other hexes should swap to hexit_first.
                for (i = 0; i<6; ++i) {
                    if (hexit_first->has_neighbour(i)) {
                        // For each neighbour to hexit_first, check its centre is one long-radius from v.v. Only two will fulfil this criterion.
                        Flt x_ = hexit_first->get_neighbour(i)->x - v_init.first;
                        Flt y_ = hexit_first->get_neighbour(i)->y - v_init.second;
                        Flt distance = sqrt (x_*x_ + y_*y_);
                        DBG2 ("vertex to hex-centre distance: " << distance);
                        DBG2 ("HexGrid long radius distance: " << hexit_first->getLR());
                        bool correct_distance = distance-hexit_first->getLR() < 0.001 ? true : false;
                        if (correct_distance) {
                            if (f[hexit_first->get_neighbour(i)->vi] != edgedoms.second
                                && f[hexit_first->get_neighbour(i)->vi] != edgedoms.first) {
                                DBG ("Found the true hexit_first");
                                hexit = hexit_first;
                                hexit_first = hexit_first->get_neighbour(i);

                                DBG ("UPDATE: Initial hex has f=" << f[hexit_first->vi] << ", " << hexit_first->outputRG() << " and initial neighbours:");
                                for (int k = 0; k<6; ++k) {
                                    DBG (Hex::neighbour_pos(k) << ": " << f[hexit_first->get_neighbour (k)->vi]);
                                }

                                break;
                            }
                        }
                    }
                }
            }

            // Now the main walking algorithm
            while (!partner_found) {

                DBG ("===== while loop. partner_found==false ======");

                // Find the initial direction of the edge and the hex containing edgedoms.first:
                int hexit_first_dirn = numeric_limits<int>::max();
                for (i = 0; i<6; ++i) {
                    DBG ("i=" << i << ", Comparing coordinates: ("
                         << hexit_first->get_vertex_coord(i).first << "," << hexit_first->get_vertex_coord(i).second
                         << ") and v_init = (" << v_init.first << "," << v_init.second << ")");

                    if (hexit_first->compare_vertex_coord(i, v_init) == true) {

                        // Then the neighbours are either side of vertex direction i.
                        DBG ("initial *vertex* in direction " << i << "/" << Hex::vertex_name(i));

                        if (hexit_first->has_neighbour ((i+1)%6)) {
                            DBG ("Hex adjoining " << hexit_first->outputRG() << " to " << Hex::neighbour_pos((i+1)%6)
                                 << " has f=" << f[hexit_first->get_neighbour ((i+1)%6)->vi] << ", " << hexit_first->get_neighbour ((i+1)%6)->outputRG());

                            // The next hex to be pointed to by hexit is the one with f==edgedoms.first
                            hexit = hexit_first->get_neighbour ((i+1)%6);

                            if (f[hexit->vi] == edgedoms.first) {
                                hexit_first_dirn = (i+1)%6;
                                DBG ("Good, hex in direction "
                                     << Hex::neighbour_pos(hexit_first_dirn) << ", which is " << hexit->outputRG()
                                     << ", has ID = edgedoms.first = " << edgedoms.first << " (edgedoms.second=" << edgedoms.second << ")");
                                break;
                            } else {
                                DBG ("Hex in direction " << Hex::neighbour_pos((i+1)%6) << " has ID!=edgedoms.first = " << edgedoms.first);
                            }
                        } // else No neighbour in direction " << ((i+1)%6) << " of a vertex hex. What to do?"

                        if (hexit_first->has_neighbour ((i>0)?(i-1):5)) {
                            DBG ("Hex adjoining " << hexit_first->outputRG() << " to " << Hex::neighbour_pos((i>0)?(i-1):5)
                                 << " has f=" << f[hexit_first->get_neighbour ((i>0)?(i-1):5)->vi] << ", " << hexit_first->get_neighbour ((i>0)?(i-1):5)->outputRG());

                            // The next hex to be pointed to by hexit is the one with f==edgedoms.first
                            hexit = hexit_first->get_neighbour ((i>0)?(i-1):5);

                            if (f[hexit->vi] == edgedoms.first) {
                                hexit_first_dirn = (i>0)?(i-1):5;
                                DBG ("Good, hex in direction "
                                     << Hex::neighbour_pos(hexit_first_dirn) << ", which is " << hexit->outputRG()
                                     << ", has ID = edgedoms.first = " << edgedoms.first << " (edgedoms.second=" << edgedoms.second << ")");
                                break;
                            } else {
                                DBG ("Hex in direction " << Hex::neighbour_pos((i>0)?(i-1):5) << " has ID!=edgedoms.first = " << edgedoms.first);
                            }
                        } else {
                            // If we get here, then neither hex to each
                            // side of the initial hexes were on the
                            // edge. That means that the edge has length 2
                            // vertices only.
                            hexit_first_dirn = (i>0)?(i-1):5;
                            DBG ("Okay, hex in direction "
                                 << Hex::neighbour_pos(hexit_first_dirn) << ", which is " << hexit->outputRG()
                                 << ", has neither ID. Only 2 vertices in the edge.");
                            break;
                        }

                    }
                }
                DBG ("After determining initial direction of edge, i=" << i << " or vertex dirn: " << Hex::vertex_name(i));
                DBG ("...and direction to edgedoms.first Hex is " << hexit_first_dirn << " or hex dirn: " << Hex::neighbour_pos(hexit_first_dirn));

                // Now point hexit_neighb at the hex_first containing edgedoms.second_first
                // Look at hex neighbours in directions i+1 and i-1 from hexit_first.
                bool found_second = false;
                int hexit_second_dirn = numeric_limits<int>::max(); // dirn from hexit_first to hexit_neighb which has edgedoms.second identity
                j = (hexit_first_dirn+1)%6;
                if (hexit_first->has_neighbour (j)) {
                    // If we have a neighbour, then check if it's on the other side of the edge; i.e. that the initial vertex
                    // v.v lies between the neighbour hexit->get_neighbour(j) and hexit.
                    hexit_neighb = hexit_first->get_neighbour(j);
                    DBG ("hexit_neighb, which should be over the edge has f="
                         << f[hexit_neighb->vi] << ", " << hexit_neighb->outputRG()
                         << ". Comparing with edgedoms.second=" << edgedoms.second);
                    if (f[hexit_neighb->vi] == edgedoms.second) { // Might need to match against edgedoms.first in walk_next
                        found_second = true;
                        hexit_second_dirn = j;
                    }
                }
                if (!found_second) {
                    j = (hexit_first_dirn>0)?(hexit_first_dirn-1):5;
                    if (hexit_first->has_neighbour (j)) {
                        hexit_neighb = hexit_first->get_neighbour(j);
                        DBG ("the other hexit_neighb, which should be over the edge has f="
                             << f[hexit_neighb->vi] << ", " << hexit_neighb->outputRG());
                        if (f[hexit_neighb->vi] == edgedoms.second) {
                            found_second = true;
                            hexit_second_dirn = j;
                        }
                    }
                }
                if (!found_second) {
                    throw runtime_error ("Whoop whoop - failed to find the second hex associated with the initial vertex!");
                }

                // Can now say whether the edgedoms are in clockwise or anti-clockwise order.
                Rotn rot = Rotn::Unknown;
                int hex_hex_neighb_dirn = numeric_limits<int>::max();
                if (hexit_second_dirn == ((hexit_first_dirn>0)?(hexit_first_dirn-1):5)) {
                    // Rotation of edgedoms.first to edgedoms.second is clockwise around hexit_first.

                    // Direction from hexit to hexit_neighb is the hexit anti-direction + 1
                    hex_hex_neighb_dirn = (((hexit_first_dirn+3)%6)+1)%6;

                    // Rotation from hexit_first to hexit_neighb is therefore ANTI-clockwise.
                    DBG ("Rotate anticlockwise around hexit");
                    rot = Rotn::Anticlock;

                } else if (hexit_second_dirn  == (hexit_first_dirn+1)%6) {
                    // Rotation of edgedoms.first to edgedoms.second is anti-clockwise around hexit_first

                    // Direction from hexit to hexit_neighb is the hexit anti-direction - 1
                    hex_hex_neighb_dirn = (hexit_first_dirn+3)%6;
                    hex_hex_neighb_dirn = hex_hex_neighb_dirn>0?(hex_hex_neighb_dirn-1):5;

                    // Rotation from hexit_first to hexit_neighb is therefore CLOCKWISE.
                    DBG ("Rotate clockwise around hexit");
                    rot = Rotn::Clock;

                } // else rot == Rotn::Unknown


                // Now hexit and hexit_neighb hexes straddle the edge that
                // I want to walk along, and I know which way to rotate
                // around hexit to find all the edge vertices that
                // surround hexit.
                // It should be that case that hexit_neighb == hexit->get_neighbour(hex_hex_neighb_dirn);
                if (hexit_neighb == hexit->get_neighbour(hex_hex_neighb_dirn)) {
                    DBG ("hexit_neighb is in the right direction.");
                }

                // Here, we have in hexit, a hex with value
                // edgedoms.first. Find the neighbour hex with value
                // edgedoms.second and add two vertices to v.edge accordingly.

                // Rotate all the way around each "inner edge hex", starting from hex_hex_neighb_dirn.
                // Rotation may be clockwise or anticlockwise.
                int last_j;
                if (rot == Rotn::Anticlock) {
                    last_j = hex_hex_neighb_dirn>0?(hex_hex_neighb_dirn-1):5;
                } else {
                    last_j = (hex_hex_neighb_dirn+1)%6;
                }

                // This for loop rotates around each inner edge hexit:
                hexit_last = hexit_neighb;
                for (j  = hex_hex_neighb_dirn;
                     j != last_j;
                     j  = (rot == Rotn::Anticlock)?((j+1)%6):(j>0?j-1:5) ) {

                    if (hexit->has_neighbour (j)) {
                        // If we have a neighbour, then check if it's on the other side of the edge.
                        hexit_next = hexit->get_neighbour (j);

                        DBG ("hexit_next, " << hexit_next->outputRG() << ", which should be over the edge has f=" << f[hexit_next->vi]);
                        if (f[hexit_next->vi] == edgedoms.second) {
                            // hexit_next has identity edgedoms.second, so add vertex j
                            DBG ("push_back vertex coordinate " << (j>0?j-1:5) << " for the path");
                            v_init = hexit->get_vertex_coord (j>0?j-1:5);
                            path.push_back (v_init);
                            // Update hexit_last
                            hexit_last = hexit_next;

                        } else {
                            DBG ("This neighbour does not have identity = edgedoms.second = " << edgedoms.second);
                            if (f[hexit_next->vi] == edgedoms.first) {
                                DBG ("This neighbour DOES have identity = edgedoms.first = " << edgedoms.first);

                                DBG ("push_back next vertex coordinate " << (j>0?j-1:5) << " for the path");
                                v_init = hexit->get_vertex_coord (j>0?j-1:5);
                                path.push_back (v_init);

                                // This is the time to cycle around the hexes
                                DBG ("Setting up next hexits...");
                                DBG ("Set hexit_first to " << hexit->outputRG());
                                hexit_first = hexit;
                                DBG ("Set hexit to " << hexit_next->outputRG());
                                hexit = hexit_next;
                                DBG ("Set hexit_neighb to " << hexit_last->outputRG());
                                hexit_neighb = hexit_last; // The last neighbour with identity edgedoms.second

                                // Update v_dirn ready for the next loop
                                //i = (rot == Rotn::Anticlock)?((j+1)%6):(j>0?(j-1):5);
                                //DBG ("Setting v_init to ");
                                //v_init = hexit->get_vertex_coord (i);

                                break;
                            } else {
                                DBG ("Not either of the edgedom identities, must be end of the edge");
                                DBG ("push_back final vertex coordinate " << (j>0?j-1:5) << " for the path");
                                v_init = hexit->get_vertex_coord (j>0?j-1:5);
                                path.push_back (v_init);
                                next_one = v_init;
                                next_neighb_dom = f[hexit_next->vi];
                                partner_found = true;
                                break;
                            }
                        }
                    }
                }
            } // end while !partner_found

            return next_one;
        }

        /*!
         * Walk out to the next vertex from vertx @v on HexGrid @hg
         * for which identities are in @f.
         *
         * @hg The HexGrid on which the action takes place
         *
         * @f The identity variable.
         *
         * @v The Dirichlet Vertex that we're walking from, and into
         * which we're going to write the path to the next neighbour
         *
         * @next_neighb_dom The identity of the next domain neighbour
         * when we find the vertex neighbour.
         */
        static pair<Flt, Flt>
        walk_to_next (HexGrid* hg, vector<Flt>& f, DirichVtx<Flt>& v, Flt& next_neighb_dom) {

            DBG ("Called");

            // Starting from hex v.hi, find neighbours whos f
            // values are v.f/v.neighb.first. Record
            // (in v.path_to_next) a series of coordinates that make up
            // the path between that vertex and the next vertex in the domain.
            pair<Flt, Flt> edgedoms;
            edgedoms.first = v.f;
            edgedoms.second = v.neighb.first;

            return walk_common (hg, f, v, v.pathto_neighbour, edgedoms, next_neighb_dom);
        }

        /*!
         * Walk out to a neighbour from vertex @v.
         *
         * @hg The HexGrid on which the action takes place
         *
         * @f The identity variable.
         *
         * @v The Dirichlet Vertex that we're walking from, and into
         * which we're going to write the path to the next neighbour
         *
         * @next_neighb_dom The identity of the next domain neighbour
         * when we find the vertex neighbour.
         */
        static pair<Flt, Flt>
        walk_to_neighbour (HexGrid* hg, vector<Flt>& f, DirichVtx<Flt>& v, Flt& next_neighb_dom) {

            DBG ("Called");

            // Don't set neighbours for the edge vertices (though
            // edge vertices *can be set* as neighbours for other
            // vertices).
            if (v.neighb.first == -1.0f || v.neighb.second == -1.0f) {
                return;
            }

            pair<Flt, Flt> edgedoms = v.neighb;

            return walk_common (hg, f, v, v.pathto_next, edgedoms, next_neighb_dom);
        }

        /*!
         * Given an iterator into the list of DirichVtxs @vertices,
         * find the next vertex in the domain, along with the vertex
         * neighbours, and recurse until @domain has been populated
         * with all the vertices that define it.
         *
         * Return true for success, false for failure, and leave dv
         * pointing to the next vertex in vertices so that @domain can
         * be stored, reset and the next Dirichlet domain can be
         * found.
         */
        static bool
        process_domain (HexGrid* hg, vector<Flt>& f,
                        typename list<DirichVtx<Flt>>::iterator dv,
                        list<DirichVtx<Flt>>& vertices,
                        list<DirichVtx<Flt>>& domain,
                        DirichVtx<Flt> first_vtx) {

            DBG ("Called");

            // Domain ID is set in dv as dv->f;
            DirichVtx<Flt> v = *dv;

            // On the first call, first_vtx should have been set to vertices.end()
            if (first_vtx.unset()) {
                // Mark the first vertex in our domain
                DBG ("Mark first vertex");
                first_vtx = v;
            }

            // Find the neighbour of this vertex, if possible. Can't
            // do this if it's a boundary vertex, but nothing happens
            // in that case.
            Flt next_neighb_dom = numeric_limits<Flt>::max();
            DBG ("walk_to_neighbour...");
            pair<Flt, Flt> neighb_vtx = walk_to_neighbour (hg, f, v, next_neighb_dom);
            DBG ("walk_to_neighbour returned with vertex (" << neighb_vtx.first << "," << neighb_vtx.second << ")");

            // Walk to the next vertex
            next_neighb_dom = numeric_limits<Flt>::max();
            pair<Flt, Flt> next_vtx = walk_to_next (hg, f, v, next_neighb_dom);
            DBG ("walk_to_next returned with vertex (" << next_vtx.first << "," << next_vtx.second << ")");

            // Erase the vertex and push back v
            dv = vertices.erase (dv);
            domain.push_back (v);

            if (first_vtx.compare (next_vtx) == false) {
                // Find dv which matches next_vtx.
                DBG ("Spin...");
                while (dv != vertices.end()) {
                    // Instead of: dv = next_vtx;, do:
                    if (dv->compare (next_vtx) == true) {
                        // vertex has correct coordinate. Check it has correct neighbours.
                        DBG ("Vertex with correct coordinate...");
                        DBG ("Is dv->f == v.f? " << dv->f << " == " << v.f << "?");
                        DBG ("Is dv->neighb.first == v.neighb.second? " << dv->neighb.first << " == " << v.neighb.second << "?");
                        DBG ("Is dv->neighb.second == next_neighb_dom? " << dv->neighb.second << " == " << next_neighb_dom << "?");
                        if (dv->f == v.f
                            && dv->neighb.first == v.neighb.second
                            && dv->neighb.second == next_neighb_dom) {
                            // Match for current dv
                            DBG ("Match");
                            break;
                        }
                    }
                    ++dv;
                }
                DBG ("I spun");
            } else {
                cout << "Walk to next arrived back at the first vertex." << endl;
                return true;
            }

            // Now delete dv and move on to the next, recalling
            // process_domain recursively, or exiting if we got to the
            // start of the domain perimeter.
            DBG ("Recursively call process_domain...");
            return (process_domain (hg, f, dv, vertices, domain, first_vtx));
        }

        /*!
         * Determine the locations of the vertices on a Hex grid which
         * are surrounded by three different values of @f. @f is
         * indexed by the HexGrid @hg. Return a list containing lists
         * of the vertices, each of which define a domain.
         */
        static list<list<DirichVtx<Flt> > >
        dirichlet_vertices (HexGrid* hg, vector<Flt>& f) {

            // 1. Go though and find a list of all vertices, in no
            // particular order.  This will lead to duplications
            // because >1 domain for a given ID, f, is possible early
            // in simulations. From this list, I can find vertex sets,
            // whilst deleting from the list until it is empty, and
            // know that I will have discovered all the domain vertex
            // sets.
            list<DirichVtx<Flt> > vertices;
            list<Hex>::iterator h = hg->hexen.begin();
            while (h != hg->hexen.end()) {
                vertex_test (hg, f, h, vertices);
                // Move on to the next Hex in hexen
                ++h;
            }

            // 2. Delete from the list<DirichVtx> and construct a
            // list<list<DirichVtx>> of all the domains. The
            // list<DirichVtx> for a single domain should be ordered
            // so that the perimeter of the domain is traversed. I
            // have to do Dirichlet domain boundary walks to achieve
            // this (to disambiguate between vertices from separate,
            // but same-ID domains).
            list<list<DirichVtx<Flt>>> dirich_domains;
            typename list<DirichVtx<Flt>>::iterator dv = vertices.begin();
            while (dv != vertices.end()) {
                list<DirichVtx<Flt>> one_domain;
                DirichVtx<Flt> first_vtx;
                bool success = process_domain (hg, f, dv, vertices, one_domain, first_vtx);
                if (success) {
                    dirich_domains.push_back (one_domain);
                } else {
                    cout << "UH OH, process_domain failed!" << endl;
                }
            }

            return dirich_domains;
        }

        /*!
         * Save all the information contained in a set of dirichlet
         * vertices to HDF5 into the HdfData @data. The set(list?) of
         * Dirichlet vertices is for one single Dirichlet domain.
         */
        static void
        dirichlet_save_vertex_set (HdfData& data /* container of domains */) {

        }

    }; // ShapeAnalysis

} // namespace morph

#endif // SHAPEANALYSIS