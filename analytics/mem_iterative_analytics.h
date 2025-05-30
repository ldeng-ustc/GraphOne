#pragma once
#include <omp.h>
#include <algorithm>

#include "graph_view.h"

using std::min;

typedef float rank_t; 
extern float qthread_dincr(float* sum, float value);
extern double qthread_doubleincr(double *operand, double incr);
template<class T, class U>
void print_bfs_summary(T* status, U level, vid_t v_count);

template<class T>
void
mem_hop1(gview_t<T>* snaph)
{
    srand(0);
    int query_count = 2048;
    vid_t  v_count  = snaph->get_vcount();
    vid_t* query = (vid_t*)calloc(sizeof(vid_t), query_count);
    int i1 = 0;
    while (i1 < query_count) {
        query[i1] = rand()% v_count;
        if (snaph->get_degree_out(query[i1]) != 0) { ++i1; };
    }

    index_t          sum = 0;
    index_t         sum1 = 0;
    index_t         sum2 = 0;
    cout << "starting 1 HOP" << endl;
	double start = mywtime();
    
    //#pragma omp parallel
    {
    degree_t      delta_degree = 0;
    degree_t        nebr_count = 0;
    degree_t      local_degree = 0;

    sid_t sid = 0;
    vid_t v   = 0;
    delta_adjlist_t<T>* delta_adjlist;
    T* local_adjlist = 0;

    //#pragma omp for reduction(+:sum) schedule (static) nowait
    for (int i = 0; i < query_count; i++) {
        
        v = query[i];
        delta_adjlist = snaph->get_nebrs_archived_out(v);
        
        if (0 != delta_adjlist) {
            nebr_count = snaph->get_degree_out(v);
            
            //traverse the delta adj list
            delta_degree = nebr_count;
            
            while (delta_adjlist != 0 && delta_degree > 0) {
                local_adjlist = delta_adjlist->get_adjlist();
                local_degree = delta_adjlist->get_nebrcount();
                degree_t i_count = min(local_degree, delta_degree);
                for (degree_t i = 0; i < i_count; ++i) {
                    sid = get_sid(local_adjlist[i]);
                    sum += sid;
                }
                delta_adjlist = delta_adjlist->get_next();
                delta_degree -= local_degree;
            }
        }

        //on-the-fly snapshots should process this
        vid_t src, dst; 
        v = query[i];
        edgeT_t<T>* edges = 0;
        index_t marker = snaph->get_nonarchived_edges(edges);
        if (0 == marker) continue;
        #pragma omp parallel for reduction(+:sum1) schedule(static)
        for (index_t j = 0; j < marker ; ++j) {
            src = TO_SID(edges[j].src_id);
            dst = TO_SID(get_sid(edges[j].dst_id));
            if (src == v) {
                sum1 += dst;
            }

            if (dst == v) {
                sum1 += src;
            }
        }
    }
    }
    
    sum += sum1;
    sum2 += sum;
    double end = mywtime();

    cout << "Sum = " << sum << " 1 Hop Time = " << end - start << endl;
}

/*
template<class T>
void
mem_hop1(vert_table_t<T>* graph_out, degree_t* degree_out, 
        snapshot_t* snapshot, index_t marker, edgeT_t<T>* edges,
        vid_t v_count)
{
    index_t         old_marker = 0;

    if (snapshot) { 
        old_marker = snapshot->marker;
    }
    
    srand(0);
    int query_count = 2048;
    vid_t* query = (vid_t*)calloc(sizeof(vid_t), query_count);
    int i1 = 0;
    while (i1 < query_count) {
        query[i1] = rand()% v_count;
        if (degree_out[query[i1]] != 0) { ++i1; };
    }

    index_t          sum = 0;
    index_t         sum1 = 0;
    index_t         sum2 = 0;
    cout << "starting 1 HOP" << endl;
	double start = mywtime();
    
    #pragma omp parallel
    {
    degree_t      delta_degree = 0;
    degree_t    durable_degree = 0;
    degree_t        nebr_count = 0;
    degree_t      local_degree = 0;

    sid_t sid = 0;
    vid_t v   = 0;
    vert_table_t<T>* graph  = graph_out;
    delta_adjlist_t<T>* delta_adjlist;
    vunit_t<T>* v_unit = 0;
    T* local_adjlist = 0;

    #pragma omp for reduction(+:sum) schedule (static) nowait
    for (int i = 0; i < query_count; i++) {
        
        v = query[i];
        v_unit = graph[v].get_vunit();
        if (0 == v_unit) continue;

        durable_degree = v_unit->count;
        delta_adjlist = v_unit->delta_adjlist;
        nebr_count = degree_out[v];
        
        //traverse the delta adj list
        delta_degree = nebr_count - durable_degree;
        
        while (delta_adjlist != 0 && delta_degree > 0) {
            local_adjlist = delta_adjlist->get_adjlist();
            local_degree = delta_adjlist->get_nebrcount();
            degree_t i_count = min(local_degree, delta_degree);
            for (degree_t i = 0; i < i_count; ++i) {
                sid = get_nebr(local_adjlist, i);
                sum += sid;
            }
            delta_adjlist = delta_adjlist->get_next();
            delta_degree -= local_degree;
        }
        assert(0 == durable_degree);
    }

    //on-the-fly snapshots should process this
    vid_t src, dst; 
    #pragma omp for reduction(+:sum1) nowait schedule(static)
    for (index_t j = old_marker; j < marker; ++j) {
        for (int i = 0; i < query_count; i++) {
            v = query[i];
            src = edges[j].src_id;
            dst = edges[j].dst_id;
            if (src == v) {
                sum1 += dst;
            }

            if (dst == v) {
                sum1 += src;
            }
        }
    }
    }
    sum += sum1;
    sum2 += sum;
    double end = mywtime();

    cout << "Sum = " << sum << " 1 Hop Time = " << end - start << endl;
}
*/
class hop2_t {
 public:
     vid_t vid;
     degree_t d;
     vid_t* vlist;
};

/*
template<class T>
void
mem_hop2(vert_table_t<T>* graph_out, degree_t* degree_out, 
        snapshot_t* snapshot, index_t marker, edgeT_t<T>* edges,
        vid_t v_count)
{
    index_t         old_marker = 0;
    if (snapshot) { 
        old_marker = snapshot->marker;
    }
    
    srand(0);
    int query_count = 512;
    hop2_t* query = (hop2_t*)calloc(sizeof(hop2_t), query_count); 
    int i1 = 0;
    while (i1 < query_count) {
        query[i1].vid = rand()% v_count;
        if (degree_out[query[i1].vid] != 0) { ++i1; };
    }

	double start = mywtime();

    #pragma omp parallel
    {    
    degree_t      delta_degree = 0;
    degree_t    durable_degree = 0;
    degree_t        nebr_count = 0;
    degree_t      local_degree = 0;

    sid_t sid = 0;
    vid_t v   = 0;
    vert_table_t<T>* graph  = graph_out;
    T* local_adjlist = 0;
    delta_adjlist_t<T>* delta_adjlist;
    vunit_t<T>* v_unit = 0;
    degree_t d = 0;
    vid_t* vlist = 0;
    
    //first hop------------------
    #pragma omp for schedule(static) nowait
    for (int q = 0; q < query_count; q++) {
        d = 0; 
        v = query[q].vid;
        nebr_count = degree_out[v];
        vlist = (vid_t*)calloc(sizeof(vid_t), nebr_count);
        query[q].vlist = vlist;
        v_unit = graph[v].get_vunit();
        if (0 == v_unit) continue;

        durable_degree = v_unit->count;
        delta_adjlist = v_unit->delta_adjlist;
        
        //traverse the delta adj list
        delta_degree = nebr_count - durable_degree;
        
        while (delta_adjlist != 0 && delta_degree > 0) {
            local_adjlist = delta_adjlist->get_adjlist();
            local_degree = delta_adjlist->get_nebrcount();
            degree_t i_count = min(local_degree, delta_degree);
            for (degree_t i = 0; i < i_count; ++i) {
                sid = get_nebr(local_adjlist, i);
                vlist[d] = sid;
                ++d;
            }
            delta_adjlist = delta_adjlist->get_next();
            delta_degree -= local_degree;
        }
        assert(durable_degree == 0);
        __sync_fetch_and_add(&query[q].d, d);
    }
    
    //on-the-fly snapshots should process this

    vid_t src, dst;
    degree_t d1 = 0;
    #pragma omp for schedule(static) nowait 
    for (index_t i = old_marker; i < marker; ++i) {
        src = edges[i].src_id;
        dst = edges[i].dst_id;
        for (int q = 0; q < query_count; q++) {
            vlist = query[q].vlist;
            v = query[q].vid;
            if (src == v) {
                d1 = __sync_fetch_and_add(&query[q].d, 1);
                vlist[d1] = dst;
            }

            if (dst == v) {
                d1 = __sync_fetch_and_add(&query[q].d, 1);
                vlist[d1] = src;
            }
        }
    }
    }
    
    //Second hop------------------
    index_t     sum = 0;
    index_t    sum1 = 0;
    index_t    sum2 = 0;
    
    #pragma omp parallel  
    {
    degree_t      delta_degree = 0;
    degree_t    durable_degree = 0;
    degree_t        nebr_count = 0;
    degree_t      local_degree = 0;
    sid_t sid = 0;
    vid_t v = 0;
    vert_table_t<T>* graph  = graph_out;
    T* local_adjlist = 0;
    delta_adjlist_t<T>* delta_adjlist;
    vunit_t<T>* v_unit = 0;
    vid_t* vlist = 0;
    degree_t d = 0;
    
    #pragma omp for schedule (static) reduction(+:sum) nowait
    for (int q = 0; q < query_count; q++) {
        vlist = query[q].vlist;
        d = query[q].d;

        for (vid_t j = 0; j < d; ++j) {
            v = vlist[j];
            v_unit = graph[v].get_vunit();
            if (0 == v_unit) continue;

            durable_degree = v_unit->count;
            delta_adjlist = v_unit->delta_adjlist;
            nebr_count = degree_out[v];
            
            //traverse the delta adj list
            delta_degree = nebr_count - durable_degree;
            
            while (delta_adjlist != 0 && delta_degree > 0) {
                local_adjlist = delta_adjlist->get_adjlist();
                local_degree = delta_adjlist->get_nebrcount();
                degree_t i_count = min(local_degree, delta_degree);
                for (degree_t i = 0; i < i_count; ++i) {
                    sid = get_nebr(local_adjlist, i);
                    sum += sid;
                }
                delta_adjlist = delta_adjlist->get_next();
                delta_degree -= local_degree;
            }
            assert(durable_degree == 0);
        }
    }

    //on-the-fly snapshots should process this
    vid_t src, dst;
    #pragma omp for reduction(+:sum1) schedule(static) nowait
    for (index_t i = old_marker; i < marker; ++i) {
        src = edges[i].src_id;
        dst = edges[i].dst_id;
        for (int q = 0; q < query_count; q++) {
            d = query[q].d;
            vlist = query[q].vlist;
            for (degree_t j = 0; j < d; ++j) {
                v = vlist[j];
                if (src == v) {
                    sum1 += dst;
                }

                if (dst == v) {
                    sum1 += src;
                }
            }
        }
    }
    }
        
    //if(vlist) free(vlist);
    //vlist = 0;
    sum2 += sum1;
    sum2 += sum; 

    double end = mywtime();
    free(query);
    cout << "Sum = " << sum2 << " 2 Hop Time = " << end - start << endl;
}
*/

template<class T>
void mem_hop2(gview_t<T>* snaph) 
{
    srand(0);
    int query_count = 512;
    vid_t v_count = snaph->get_vcount();
    hop2_t* query = (hop2_t*)calloc(sizeof(hop2_t), query_count); 
    int i1 = 0;
    while (i1 < query_count) {
        query[i1].vid = rand()% v_count;
        if (snaph->get_degree_out(query[i1].vid) != 0) { ++i1; };
    }

	double start = mywtime();
    index_t     sum = 0;
    index_t    sum1 = 0;
    index_t    sum2 = 0;

        
    degree_t      delta_degree = 0;
    degree_t        nebr_count = 0;
    degree_t      local_degree = 0;

    sid_t sid = 0;
    vid_t v   = 0;
    T* local_adjlist = 0;
    delta_adjlist_t<T>* delta_adjlist;
    degree_t d = 0;
    vid_t* vlist = 0;
    
    //first hop------------------
    for (int q = 0; q < query_count; q++) {
        d = 0; 
        v = query[q].vid;
        nebr_count = snaph->get_degree_out(v);
        vlist = (vid_t*)calloc(sizeof(vid_t), nebr_count);
        query[q].vlist = vlist;
        delta_adjlist = snaph->get_nebrs_archived_out(v);
        if (0 == delta_adjlist) continue;
        
        //traverse the delta adj list
        delta_degree = nebr_count;
        
        while (delta_adjlist != 0 && delta_degree > 0) {
            local_adjlist = delta_adjlist->get_adjlist();
            local_degree = delta_adjlist->get_nebrcount();
            degree_t i_count = min(local_degree, delta_degree);
            for (degree_t i = 0; i < i_count; ++i) {
                sid = get_sid(local_adjlist[i]);
                vlist[d] = sid;
                ++d;
            }
            delta_adjlist = delta_adjlist->get_next();
            delta_degree -= local_degree;
        }
        query[q].d += d;

        //on-the-fly snapshots should process this
        edgeT_t<T>* edges = 0;
        index_t marker = snaph->get_nonarchived_edges(edges);
        #pragma omp parallel
        {
            vid_t src, dst;
            degree_t d1 = 0;
            vid_t v = query[q].vid;
            vid_t* vlist = query[q].vlist;
            #pragma omp for schedule(static) nowait 
            for (index_t i = 0; i < marker; ++i) {
                src = TO_SID(edges[i].src_id);
                dst = TO_SID(get_dst(edges[i]));
                if (src == v) {
                    d1 = __sync_fetch_and_add(&query[q].d, 1);
                    vlist[d1] = dst;
                }

                if (dst == v) {
                    d1 = __sync_fetch_and_add(&query[q].d, 1);
                    vlist[d1] = src;
                }
            }
        }
    
    
        //Second hop------------------
        sum = 0;
        sum1 = 0;
        #pragma omp parallel
        {
        degree_t      delta_degree = 0;
        degree_t        nebr_count = 0;
        degree_t      local_degree = 0;
        sid_t sid = 0;
        vid_t v = 0;
        T* local_adjlist = 0;
        delta_adjlist_t<T>* delta_adjlist;
        vid_t* vlist = 0;
        degree_t d = 0;
        
         vlist = query[q].vlist;
         d = query[q].d;

        #pragma omp for schedule (static) reduction(+:sum) nowait
        for (degree_t j = 0; j < d; ++j) {
            delta_adjlist = snaph->get_nebrs_archived_out(v); 
            if (0 == delta_adjlist) continue;

            nebr_count = snaph->get_degree_out(v);
            
            //traverse the delta adj list
            delta_degree = nebr_count;
            
            while (delta_adjlist != 0 && delta_degree > 0) {
                local_adjlist = delta_adjlist->get_adjlist();
                local_degree = delta_adjlist->get_nebrcount();
                degree_t i_count = min(local_degree, delta_degree);
                for (degree_t i = 0; i < i_count; ++i) {
                    sid = get_sid(local_adjlist[i]);
                    sum += sid;
                }
                delta_adjlist = delta_adjlist->get_next();
                delta_degree -= local_degree;
            }
        }

        //on-the-fly snapshots should process this
        vid_t src, dst;
        edgeT_t<T>* edges = 0;
        index_t marker = snaph->get_nonarchived_edges(edges);
        #pragma omp for reduction(+:sum1) schedule(static) nowait
        for (index_t i = 0; i < marker; ++i) {
            src = TO_SID(edges[i].src_id);
            dst = TO_SID(get_dst(edges[i]));
            for (degree_t j = 0; j < d; ++j) {
                v = vlist[j];
                if (src == v) {
                    sum1 += dst;
                }

                if (dst == v) {
                    sum1 += src;
                }
            }
        }
        }
        sum2 += sum1;
        sum2 += sum; 
    }
        
    //if(vlist) free(vlist);
    //vlist = 0;

    double end = mywtime();
    free(query);
    cout << "Sum = " << sum2 << " 2 Hop Time = " << end - start << endl;
}

template<class T>
void mem_bfs_simple(gview_t<T>* snaph,
        uint8_t* status, sid_t root)
{
	int				level      = 1;
	int				top_down   = 1;
	sid_t			frontier   = 0;
    sid_t           v_count    = snaph->get_vcount();

	double start1 = mywtime();
    
    //if (snaph->degree_out[root] == 0) { root = 0;}
	status[root] = level;
    
	do {
		frontier = 0;
		//double start = mywtime();
		#pragma omp parallel reduction(+:frontier)
		{
            sid_t sid;
            degree_t nebr_count = 0;
            degree_t prior_sz = 65536;
            T* local_adjlist = (T*)malloc(prior_sz*sizeof(T));
		    
            if (top_down) {
				
                #pragma omp for nowait
				for (vid_t v = 0; v < v_count; v++) {
					if (status[v] != level) continue;
					
                    nebr_count = snaph->get_degree_out(v);
                    if (nebr_count == 0) {
                        continue;
                    } else if (nebr_count > prior_sz) {
                        prior_sz = nebr_count;
                        free(local_adjlist);
                        local_adjlist = (T*)malloc(prior_sz*sizeof(T));
                    }

                    snaph->get_nebrs_out(v, local_adjlist);

                    for (degree_t i = 0; i < nebr_count; ++i) {
                        sid = get_sid(local_adjlist[i]);
                        if (status[sid] == 0) {
                            status[sid] = level + 1;
                            ++frontier;
                            //cout << " " << sid << endl;
                        }
                    }
				}
			} else {//bottom up
				
				#pragma omp for nowait
				for (vid_t v = 0; v < v_count; v++) {
					if (status[v] != 0 ) continue;
                    
                    nebr_count = snaph->get_degree_in(v);
                    if (nebr_count == 0) { 
                        continue;
                    } else if (nebr_count > prior_sz) {
                        prior_sz = nebr_count;
                        free(local_adjlist);
                        local_adjlist = (T*)malloc(prior_sz*sizeof(T));
                    }

                    snaph->get_nebrs_in(v, local_adjlist);

                    //traverse the delta adj list
                    for (degree_t i = 0; i < nebr_count; ++i) {
                        sid = get_sid(local_adjlist[i]);
                        if (status[sid] == level) {
                            status[v] = level + 1;
                            ++frontier;
                            break;
                        }
                    }
				}
		    }
            
            //on-the-fly snapshots should process this
            //cout << "On the Fly" << endl;
            vid_t src, dst;
            edgeT_t<T>* edges;
            index_t count = snaph->get_nonarchived_edges(edges);
            
            #pragma omp for schedule (static)
            for (index_t i = 0; i < count; ++i) {
                src = TO_SID(edges[i].src_id);
                dst = TO_SID(get_dst(edges+i));
                if (status[src] == 0 && status[dst] == level) {
                    status[src] = level + 1;
                    ++frontier;
                    //cout << " " << src << endl;
                } 
                if (status[src] == level && status[dst] == 0) {
                    status[dst] = level + 1;
                    ++frontier;
                    //cout << " " << dst << endl;
                }
            }
        }

		//double end = mywtime();
	
		//cout << "Top down = " << top_down
		//     << " Level = " << level
        //     << " Frontier Count = " << frontier
		//     << " Time = " << end - start
		//     << endl;
	
        //Point is to simulate bottom up bfs, and measure the trade-off    
		if ((frontier >= 0.002*v_count) || level == 2) {
			top_down = false;
		} else {
            top_down = true;
        }
		++level;
	} while (frontier);
		
    double end1 = mywtime();
    cout << "BFS Time = " << end1 - start1 << endl;
    print_bfs_summary(status, level, v_count);
}

template<class T, class StatusType>
void mem_bfs(gview_t<T>* snaph,
        StatusType* status, sid_t root)
{
    int				level      = 1;
	int				top_down   = 1;
	sid_t			frontier   = 0;
    sid_t           v_count    = snaph->get_vcount();
    
	double start1 = mywtime();
    //if (snaph->get_degree_out(root) == 0) { root = 0;}
	status[root] = level;
    
	do {
		frontier = 0;
		double start = mywtime();
		#pragma omp parallel reduction(+:frontier)
		{
            sid_t sid;
            degree_t nebr_count = 0;
            degree_t local_degree = 0;
            degree_t delta_degree = 0;

            delta_adjlist_t<T>* delta_adjlist;;
            T* local_adjlist = 0;
            // std::cout << "vcount: " << v_count << std::endl;
		    
            if (top_down) {
                #pragma omp for nowait
				for (vid_t v = 0; v < v_count; v++) {
                    if (status[v] != level) continue;
                    
					nebr_count     = snaph->get_degree_out(v);
                    if (0 == nebr_count) continue;
                    
                    delta_adjlist  = snaph->get_nebrs_archived_out(v);
                    delta_degree   = nebr_count;
                    
                    //traverse the delta adj list
                    while (delta_adjlist != 0 && delta_degree > 0) {
                        local_adjlist = delta_adjlist->get_adjlist();
                        local_degree = delta_adjlist->get_nebrcount();
                        degree_t i_count = min(local_degree, delta_degree);
                        for (degree_t i = 0; i < i_count; ++i) {
                            sid = get_sid(local_adjlist[i]);
                            if (status[sid] == 0) {
                                status[sid] = level + 1;
                                ++frontier;
                                //cout << " " << sid << endl;
                            }
                        }
                        delta_adjlist = delta_adjlist->get_next();
                        delta_degree -= local_degree;
                        // if(v % 13777773 ==0 && delta_degree != 0) {
                        //     cout << v << ": delta_degree = " << delta_degree << endl;
                        // }
                    }
				}
			} else {//bottom up
                int done = 0;
				
				#pragma omp for nowait
				for (vid_t v = 0; v < v_count; v++) {
					if (status[v] != 0 ) continue;
					nebr_count = snaph->get_degree_in(v);
                    if (0 == nebr_count) continue;

                    delta_adjlist  = snaph->get_nebrs_archived_in(v);
                    done = 0;

                    //traverse the delta adj list
                    delta_degree = nebr_count;
                    while (delta_adjlist != 0 && delta_degree > 0) {
                        local_adjlist = delta_adjlist->get_adjlist();
                        local_degree = delta_adjlist->get_nebrcount();
                        degree_t i_count = min(local_degree, delta_degree);
                        for (degree_t i = 0; i < i_count; ++i) {
                            sid = get_sid(local_adjlist[i]);
                            if (status[sid] == level) {
                                status[v] = level + 1;
                                ++frontier;
                                done = 1;
                                break;
                            }
                        }
                        if (done == 1) break;
                        delta_adjlist = delta_adjlist->get_next();
                        delta_degree -= local_degree;
                    }
				}
		    }
            
            //on-the-fly snapshots should process this
            //cout << "On the Fly" << endl;
            vid_t src, dst;
            edgeT_t<T>* edges;
            index_t marker = snaph->get_nonarchived_edges(edges);

            #pragma omp for schedule (static)
            for (index_t i = 0; i < marker; ++i) {
                src = TO_SID(edges[i].src_id);
                dst = TO_SID(get_dst(edges+i));
                if (status[src] == 0 && status[dst] == level) {
                    status[src] = level + 1; 
                    ++frontier;
                    //cout << " " << src << endl;
                } 
                if (status[src] == level && status[dst] == 0) {
                    status[dst] = level + 1;
                    ++frontier;
                    //cout << " " << dst << endl;
                }
            }
        }
        
        
		double end = mywtime();
        if(root == 1)
            cout << "Top down = " << top_down
                << " Level = " << level
                << " Frontier Count = " << frontier
                << " Time = " << end - start
                << endl;

        //Point is to simulate bottom up bfs, and measure the trade-off    
        if ((frontier >= 0.002*v_count) && ( 0 == snaph->is_unidir())){ // || level == 2)
			top_down = false;
		} else {
            top_down = true;
        }
		++level;
	} while (frontier);
		
    double end1 = mywtime();
    cout << "BFS root = "<< root << "Time = " << end1 - start1 << endl;
    // print_bfs_summary(status, level, v_count);
}

template<class T>
void mem_pagerank_push(gview_t<T>* snaph, int iteration_count)
{
	float* rank_array = 0 ;
	float* prior_rank_array = 0;
    float* dset = 0;
	vid_t v_count = snaph->get_vcount();
    
    double start = mywtime();
    
    rank_array  = (float*)mmap(NULL, sizeof(float)*v_count, PROT_READ|PROT_WRITE,
                            MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB|MAP_HUGE_2MB, 0, 0 );
    if (MAP_FAILED == rank_array) {
        cout << "Huge page alloc failed for rank array" << endl;
        rank_array = (float*)calloc(v_count, sizeof(float));
    }
    
    prior_rank_array  = (float*)mmap(NULL, sizeof(float)*v_count, PROT_READ|PROT_WRITE,
                            MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB|MAP_HUGE_2MB, 0, 0 );
    if (MAP_FAILED == prior_rank_array) {
        cout << "Huge page alloc failed for prior rank array" << endl;
        prior_rank_array = (float*)calloc(v_count, sizeof(float));
    }
    
    dset  = (float*)mmap(NULL, sizeof(float)*v_count, PROT_READ|PROT_WRITE,
                            MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB|MAP_HUGE_2MB, 0, 0 );
    
    if (MAP_FAILED == dset) {
        cout << "Huge page alloc failed for dset" << endl;
        dset = (float*)calloc(v_count, sizeof(float));
    }
	
	//initialize the rank, and get the degree information
    
    #pragma omp parallel
    { 
    degree_t degree = 0;
    float	inv_v_count = 0.15;//1.0f/vert_count;
    #pragma omp for
    for (vid_t v = 0; v < v_count; ++v) {
        degree = snaph->get_degree_out(v);
        if (degree != 0) {
            dset[v] = 1.0f/degree;
            prior_rank_array[v] = inv_v_count;//XXX
        } else {
            dset[v] = 0;
            prior_rank_array[v] = 0;
        }
    }
    }

	//let's run the pagerank
	for (int iter_count = 0; iter_count < iteration_count; ++iter_count) {
        double start1 = mywtime();
        #pragma omp parallel 
        {
            sid_t sid;
            degree_t      delta_degree = 0;
            degree_t nebr_count = 0;
            degree_t local_degree = 0;

            delta_adjlist_t<T>* delta_adjlist;
            T* local_adjlist = 0;

            float rank = 0.0f; 
         
            #pragma omp for schedule (dynamic, 4096) nowait 
            for (vid_t v = 0; v < v_count; v++) {
                delta_adjlist = snaph->get_nebrs_archived_out(v);
                
                nebr_count = snaph->get_degree_out(v);
                rank = prior_rank_array[v];
                
                //traverse the delta adj list
                delta_degree = nebr_count;
                while (delta_adjlist != 0 && delta_degree > 0) {
                    local_adjlist = delta_adjlist->get_adjlist();
                    local_degree = delta_adjlist->get_nebrcount();
                    degree_t i_count = min(local_degree, delta_degree);
                    for (degree_t i = 0; i < i_count; ++i) {
                        sid = get_sid(local_adjlist[i]);
                        qthread_dincr(rank_array + sid, rank);
                    }
                    delta_adjlist = delta_adjlist->get_next();
                    delta_degree -= local_degree;
                }
            }
        

            //on-the-fly snapshots should process this
            //cout << "On the Fly" << endl;
            vid_t src, dst;
            edgeT_t<T>* edges = 0;
            index_t marker = snaph->get_nonarchived_edges(edges);
            #pragma omp for 
            for (index_t i = 0; i < marker; ++i) {
                src = TO_SID(edges[i].src_id);
                dst = TO_SID(get_dst(edges+i));
                qthread_dincr(rank_array + src, prior_rank_array[dst]);
                qthread_dincr(rank_array + dst, prior_rank_array[src]);
            }

            if (iter_count != iteration_count -1) {
                #pragma omp for
                for (vid_t v = 0; v < v_count; v++ ) {
                    rank_array[v] = (0.15 + 0.85*rank_array[v])*dset[v];
                    prior_rank_array[v] = 0;
                } 
            } else { 
                #pragma omp for
                for (vid_t v = 0; v < v_count; v++ ) {
                    rank_array[v] = (0.15 + 0.85*rank_array[v]);
                    prior_rank_array[v] = 0;
                }
            }
        }
        swap(prior_rank_array, rank_array);
        double end1 = mywtime();
        cout << "Iteration Time = " << end1 - start1 << endl;
    }	
    double end = mywtime();

	cout << "PR Time = " << end - start << endl;
	cout << endl;
}

template<class T>
void mem_pagerank(gview_t<T>* snaph, int iteration_count)
{
    vid_t v_count = snaph->get_vcount();
	float* rank_array = 0 ;
	float* prior_rank_array = 0;
    float* dset = 0;
	
    double start = mywtime();
    
    rank_array  = (float*)mmap(NULL, sizeof(float)*v_count, PROT_READ|PROT_WRITE,
                            MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB|MAP_HUGE_2MB, 0, 0 );
    if (MAP_FAILED == rank_array) {
        cout << "Huge page alloc failed for rank array" << endl;
        rank_array = (float*)calloc(v_count, sizeof(float));
    }
    
    prior_rank_array  = (float*)mmap(NULL, sizeof(float)*v_count, PROT_READ|PROT_WRITE,
                            MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB|MAP_HUGE_2MB, 0, 0 );
    if (MAP_FAILED == prior_rank_array) {
        cout << "Huge page alloc failed for prior rank array" << endl;
        prior_rank_array = (float*)calloc(v_count, sizeof(float));
    }
    
    dset  = (float*)mmap(NULL, sizeof(float)*v_count, PROT_READ|PROT_WRITE,
                            MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB|MAP_HUGE_2MB, 0, 0 );
    
    if (MAP_FAILED == dset) {
        cout << "Huge page alloc failed for dset" << endl;
        dset = (float*)calloc(v_count, sizeof(float));
    }
	
	//initialize the rank, and get the degree information
    
    #pragma omp parallel
    { 
    degree_t degree = 0;
    float	inv_v_count = 0.15;//1.0f/vert_count;
    #pragma omp for
    for (vid_t v = 0; v < v_count; ++v) {
        degree = snaph->get_degree_out(v);
        if (degree != 0) {
            dset[v] = 1.0f/degree;
            prior_rank_array[v] = inv_v_count;//XXX
        } else {
            dset[v] = 0;
            prior_rank_array[v] = 0;
        }
    }
    }

	//let's run the pagerank
	for (int iter_count = 0; iter_count < iteration_count; ++iter_count) {
        double start1 = mywtime();
        #pragma omp parallel 
        {
            sid_t sid;
            degree_t      delta_degree = 0;
            degree_t nebr_count = 0;
            degree_t local_degree = 0;

            delta_adjlist_t<T>* delta_adjlist;
            T* local_adjlist = 0;

            float rank = 0.0f; 
            
            #pragma omp for schedule (dynamic, 4096) nowait 
            for (vid_t v = 0; v < v_count; v++) {
                delta_adjlist = snaph->get_nebrs_archived_in(v);
                if (0 == delta_adjlist) continue;
                
                nebr_count = snaph->get_degree_in(v);
                rank = 0.0f;
                
                //traverse the delta adj list
                delta_degree = nebr_count;
                while (delta_adjlist != 0 && delta_degree > 0) {
                    local_adjlist = delta_adjlist->get_adjlist();
                    local_degree = delta_adjlist->get_nebrcount();
                    degree_t i_count = min(local_degree, delta_degree);
                    for (degree_t i = 0; i < i_count; ++i) {
                        sid = get_sid(local_adjlist[i]);
                        rank += prior_rank_array[sid];
                    }
                    delta_adjlist = delta_adjlist->get_next();
                    delta_degree -= local_degree;
                }
                //rank_array[v] = rank;
                qthread_dincr(rank_array + v, rank);
            }
        

            //on-the-fly snapshots should process this
            //cout << "On the Fly" << endl;
            vid_t src, dst;
            edgeT_t<T>* edges = 0;
            index_t marker = snaph->get_nonarchived_edges(edges);
            #pragma omp for 
            for (index_t i = 0; i < marker; ++i) {
                src = TO_SID(edges[i].src_id);
                dst = TO_SID(get_dst(edges+i));
                qthread_dincr(rank_array + src, prior_rank_array[dst]);
                qthread_dincr(rank_array + dst, prior_rank_array[src]);
            }

            if (iter_count != iteration_count -1) {
                #pragma omp for
                for (vid_t v = 0; v < v_count; v++ ) {
                    rank_array[v] = (0.15 + 0.85*rank_array[v])*dset[v];
                    prior_rank_array[v] = 0;
                } 
            } else { 
                #pragma omp for
                for (vid_t v = 0; v < v_count; v++ ) {
                    rank_array[v] = (0.15 + 0.85*rank_array[v]);
                    prior_rank_array[v] = 0;
                }
            }
        }
        swap(prior_rank_array, rank_array);
        double end1 = mywtime();
        cout << "Iteration Time = " << end1 - start1 << endl;
    }	
    double end = mywtime();

	cout << "PR Time = " << end - start << endl;
	cout << endl;
}

template<class T>
void mem_pagerank_simple(gview_t<T>* snaph, int iteration_count)
{
    //vert_table_t<T>* graph_in; 

    //edgeT_t<T>* edges = snaph->edges;
    
    sid_t  v_count    = snaph->get_vcount();
	float* rank_array = 0 ;
	float* prior_rank_array = 0;
    float* dset = 0;
	
    double start = mywtime();
    rank_array  = (float*)mmap(NULL, sizeof(float)*v_count, PROT_READ|PROT_WRITE,
                            MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB|MAP_HUGE_2MB, 0, 0 );
    if (MAP_FAILED == rank_array) {
        cout << "Huge page alloc failed for rank array" << endl;
        rank_array = (float*)calloc(v_count, sizeof(float));
    }
    
    prior_rank_array  = (float*)mmap(NULL, sizeof(float)*v_count, PROT_READ|PROT_WRITE,
                            MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB|MAP_HUGE_2MB, 0, 0 );
    if (MAP_FAILED == prior_rank_array) {
        cout << "Huge page alloc failed for prior rank array" << endl;
        prior_rank_array = (float*)calloc(v_count, sizeof(float));
    }
    
    dset  = (float*)mmap(NULL, sizeof(float)*v_count, PROT_READ|PROT_WRITE,
                            MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB|MAP_HUGE_2MB, 0, 0 );
    
    if (MAP_FAILED == dset) {
        cout << "Huge page alloc failed for dset" << endl;
        dset = (float*)calloc(v_count, sizeof(float));
    }
	
	//initialize the rank, and get the degree information
    
    #pragma omp parallel
    { 
    degree_t degree = 0;
    float	inv_v_count = 0.15;//1.0f/vert_count;
    #pragma omp for
    for (vid_t v = 0; v < v_count; ++v) {
        degree = snaph->get_degree_out(v);
        if (degree != 0) {
            dset[v] = 1.0f/degree;
            prior_rank_array[v] = inv_v_count;//XXX
        } else {
            dset[v] = 0;
            prior_rank_array[v] = 0;
        }
    }
    }

	//let's run the pagerank
	for (int iter_count = 0; iter_count < iteration_count; ++iter_count) {
        double start1 = mywtime();
        #pragma omp parallel 
        {
            sid_t sid;
            degree_t nebr_count = 0;
            degree_t prior_sz = 65536;
            T* local_adjlist = (T*)malloc(prior_sz*sizeof(T));
            float rank = 0.0f; 
            
            #pragma omp for schedule (dynamic, 4096) nowait 
            for (vid_t v = 0; v < v_count; v++) {
                nebr_count = snaph->get_degree_in(v);
                
                if (nebr_count == 0) {
                    continue;
                } else if (nebr_count > prior_sz) {
                    prior_sz = nebr_count;
                    free(local_adjlist);
                    local_adjlist = (T*)malloc(prior_sz*sizeof(T));
                }

                rank = 0.0f;
                snaph->get_nebrs_in(v, local_adjlist);

                for (degree_t i = 0; i < nebr_count; ++i) {
                    sid = get_sid(local_adjlist[i]);
                    rank += prior_rank_array[sid];
                }
                //rank_array[v] = rank;
                qthread_dincr(rank_array + v, rank);
            }
        

            //on-the-fly snapshots should process this
            //cout << "On the Fly" << endl;
            vid_t src, dst;
            edgeT_t<T>* edges = 0;
            index_t marker = snaph->get_nonarchived_edges(edges);
            #pragma omp for 
            for (index_t i = 0; i < marker; ++i) {
                src = TO_SID(edges[i].src_id);
                dst = TO_SID(get_dst(edges+i));
                qthread_dincr(rank_array + src, prior_rank_array[dst]);
                qthread_dincr(rank_array + dst, prior_rank_array[src]);
            }

            if (iter_count != iteration_count -1) {
                #pragma omp for
                for (vid_t v = 0; v < v_count; v++ ) {
                    rank_array[v] = (0.15 + 0.85*rank_array[v])*dset[v];
                    prior_rank_array[v] = 0;
                } 
            } else { 
                #pragma omp for
                for (vid_t v = 0; v < v_count; v++ ) {
                    rank_array[v] = (0.15 + 0.85*rank_array[v]);
                    prior_rank_array[v] = 0;
                }
            }
        }
        swap(prior_rank_array, rank_array);
        double end1 = mywtime();
        cout << "Iteration Time = " << end1 - start1 << endl;
    }	
    double end = mywtime();

	cout << "PR Time = " << end - start << endl;
	cout << endl;
}

template<class T>
void 
mem_pagerank_epsilon(gview_t<T>* snaph, double epsilon)
{
    vid_t v_count = snaph->get_vcount();
	double* rank_array = 0;
	double* prior_rank_array = 0;
    double* dset = 0;
	
    double start = mywtime();
    
    rank_array = (double*)calloc(v_count, sizeof(double));
    prior_rank_array = (double*)calloc(v_count, sizeof(double));
    dset = (double*)calloc(v_count, sizeof(double));
	
	//initialize the rank, and get the degree information
    
    double	inv_count = 1.0/v_count;

    #pragma omp parallel 
    { 
    degree_t degree = 0;
    double   inv_degree = 0;
    #pragma omp for
    for (vid_t v = 0; v < v_count; ++v) {
        degree = snaph->get_degree_out(v);
        if (degree != 0) {
            inv_degree = 1.0/degree;
            dset[v] = inv_degree;
            prior_rank_array[v] = inv_count*inv_degree;
        } else {
            dset[v] = 0;
            prior_rank_array[v] = 0;
        }
    }
    }

    double  delta = 1.0;
    double	inv_v_count = 0.15/v_count;
    int iter = 0;

	//let's run the pagerank
	while(delta > epsilon) {
        //double start1 = mywtime();
        #pragma omp parallel 
        {
            sid_t sid;
            degree_t      delta_degree = 0;
            degree_t nebr_count = 0;
            degree_t local_degree = 0;

            delta_adjlist_t<T>* delta_adjlist;
            T* local_adjlist = 0;

            double rank = 0.0; 
            
            #pragma omp for 
            for (vid_t v = 0; v < v_count; v++) {
                delta_adjlist = snaph->get_nebrs_archived_in(v);
                if (0 == delta_adjlist) continue;
                
                nebr_count = snaph->get_degree_in(v);
                rank = 0.0;
                
                //traverse the delta adj list
                delta_degree = nebr_count;
                while (delta_adjlist != 0 && delta_degree > 0) {
                    local_adjlist = delta_adjlist->get_adjlist();
                    local_degree = delta_adjlist->get_nebrcount();
                    degree_t i_count = min(local_degree, delta_degree);
                    for (degree_t i = 0; i < i_count; ++i) {
                        sid = get_sid(local_adjlist[i]);
                        rank += prior_rank_array[sid];
                    }
                    delta_adjlist = delta_adjlist->get_next();
                    delta_degree -= local_degree;
                }
                rank_array[v] = rank;
            }
        
            
            double mydelta = 0;
            double new_rank = 0;
            delta = 0;
            
            #pragma omp for reduction(+:delta)
            for (vid_t v = 0; v < v_count; v++ ) {
                if (snaph->get_degree_out(v) == 0) continue;
                new_rank = inv_v_count + 0.85*rank_array[v];
                mydelta = new_rank - prior_rank_array[v]*snaph->get_degree_out(v);
                if (mydelta < 0) mydelta = -mydelta;
                delta += mydelta;

                rank_array[v] = new_rank*dset[v];
                prior_rank_array[v] = 0;
            } 
        }
        swap(prior_rank_array, rank_array);
        ++iter;
        //double end1 = mywtime();
        //cout << "Delta = " << delta << "Iteration Time = " << end1 - start1 << endl;
    }	

    #pragma omp for
    for (vid_t v = 0; v < v_count; v++ ) {
        rank_array[v] = rank_array[v]*snaph->get_degree_out(v);
    }

    double end = mywtime();

	cout << "Iteration count" << iter << endl;
    cout << "PR Time = " << end - start << endl;

    free(rank_array);
    free(prior_rank_array);
    free(dset);
	cout << endl;
}


/*
template<class T>
void mem_bfs(vert_table_t<T>* graph_out, degree_t* degree_out, 
        vert_table_t<T>* graph_in, degree_t* degree_in,
        snapshot_t* snapshot, index_t marker, edgeT_t<T>* edges,
        vid_t v_count, uint8_t* status, sid_t root)
{
	int				level      = 1;
	int				top_down   = 1;
	sid_t			frontier   = 0;
    index_t         old_marker = 0;

    if (snapshot) { 
        old_marker = snapshot->marker;
    }
    
	double start1 = mywtime();
    //if (degree_out[root] == 0) { root = 0;}
	status[root] = level;
    
	do {
		frontier = 0;
		//double start = mywtime();
		#pragma omp parallel reduction(+:frontier)
		{
            sid_t sid;
            degree_t nebr_count = 0;
            degree_t local_degree = 0;
            degree_t delta_degree = 0;

            vert_table_t<T>* graph  = 0;
            delta_adjlist_t<T>* delta_adjlist;;
            vunit_t<T>* v_unit = 0;
            T* local_adjlist = 0;
		    
            if (top_down) {
                graph  = graph_out;
				
                #pragma omp for nowait
				for (vid_t v = 0; v < v_count; v++) {
					if (status[v] != level) continue;
					v_unit = graph[v].get_vunit();
                    if (0 == v_unit) continue;

					nebr_count     = degree_out[v];
                    delta_degree   = nebr_count;
                    delta_adjlist  = v_unit->delta_adjlist;
				    //cout << "delta adjlist " << delta_degree << endl;	
				    //cout << "Nebr list of " << v <<" degree = " << nebr_count << endl;	
                    
                    //traverse the delta adj list
                    while (delta_adjlist != 0 && delta_degree > 0) {
                        local_adjlist = delta_adjlist->get_adjlist();
                        local_degree = delta_adjlist->get_nebrcount();
                        degree_t i_count = min(local_degree, delta_degree);
                        for (degree_t i = 0; i < i_count; ++i) {
                            sid = get_nebr(local_adjlist, i);
                            if (status[sid] == 0) {
                                status[sid] = level + 1;
                                ++frontier;
                                //cout << " " << sid << endl;
                            }
                        }
                        delta_adjlist = delta_adjlist->get_next();
                        delta_degree -= local_degree;
                    }
				}
			} else {//bottom up
				graph = graph_in;
                int done = 0;
				
				#pragma omp for nowait
				for (vid_t v = 0; v < v_count; v++) {
					if (status[v] != 0 ) continue;
					v_unit = graph[v].get_vunit();
                    if (0 == v_unit) continue;

                    delta_adjlist = v_unit->delta_adjlist;
					
					nebr_count = degree_in[v];
                    done = 0;

                    //traverse the delta adj list
                    delta_degree = nebr_count;
                    while (delta_adjlist != 0 && delta_degree > 0) {
                        local_adjlist = delta_adjlist->get_adjlist();
                        local_degree = delta_adjlist->get_nebrcount();
                        degree_t i_count = min(local_degree, delta_degree);
                        for (degree_t i = 0; i < i_count; ++i) {
                            sid = get_nebr(local_adjlist, i);
                            if (status[sid] == level) {
                                status[v] = level + 1;
                                ++frontier;
                                done = 1;
                                break;
                            }
                        }
                        if (done == 1) break;
                        delta_adjlist = delta_adjlist->get_next();
                        delta_degree -= local_degree;
                    }
				}
		    }

            //on-the-fly snapshots should process this
            //cout << "On the Fly" << endl;
            vid_t src, dst;
            #pragma omp for schedule (static)
            for (index_t i = old_marker; i < marker; ++i) {
                src = edges[i].src_id;
                dst = get_dst(edges+i);
                if (status[src] == 0 && status[dst] == level) {
                    status[src] = level + 1;
                    ++frontier;
                    //cout << " " << src << endl;
                } 
                if (status[src] == level && status[dst] == 0) {
                    status[dst] = level + 1;
                    ++frontier;
                    //cout << " " << dst << endl;
                }
            }
        }

		//double end = mywtime();
	
		//cout << "Top down = " << top_down
		//     << " Level = " << level
        //     << " Frontier Count = " << frontier
		//     << " Time = " << end - start
		//     << endl;
	
        //Point is to simulate bottom up bfs, and measure the trade-off    
        if ((frontier >= 0.002*v_count) || level == 2) {
			top_down = false;
		} else {
            top_down = true;
        }
		++level;
	} while (frontier);
		
    double end1 = mywtime();
    cout << "BFS Time = " << end1 - start1 << endl;
    print_bfs_summary(status, level, v_count);
}
*/

template <class T>
index_t bfs_tile(snap_t<T>* snaph, vid_t index, uint8_t* lstatus, uint8_t* rstatus, uint8_t level)
{
    header_t<T> header; 
    degree_t nebr_count = snaph->start_out(index, header);
    if (0 == nebr_count) return 0;
    
    index_t frontier = 0;
    T dst;
    snb_t snb;
    for (degree_t e = 0; e < nebr_count; ++e) {
        snaph->next(header, dst);
        snb = get_snb(dst);
        if (lstatus[snb.src] == level && 
            rstatus[snb.dst] == 0) {
            rstatus[snb.dst] = level + 1;
            ++frontier;
            //cout << " " << snb.dst + dst_offset << endl;
        }
        if (rstatus[snb.dst] == level && 
            lstatus[snb.src] == 0) {
            lstatus[snb.src] = level + 1;
            ++frontier;
            //cout << " " << snb.src + src_offset << endl;
        }
    }
    return frontier;
}

template <class T>
index_t bfs_async_tile(snap_t<T>* snaph, vid_t index, uint8_t* lstatus, uint8_t* rstatus, uint8_t level)
{
    header_t<T> header; 
    degree_t nebr_count = snaph->start_out(index, header);
    if (0 == nebr_count) return 0;
    
    index_t frontier = 0;
    T dst;
    snb_t snb;
    uint8_t llevel = 0, rlevel = 0;
    for (degree_t e = 0; e < nebr_count; ++e) {
        snaph->next(header, dst);
        snb = get_snb(dst);
        if (lstatus[snb.src] == level && 
            rstatus[snb.dst] > level+1) {
            rstatus[snb.dst] = level + 1;
            ++frontier;
            //cout << " " << snb.dst + dst_offset << endl;
        }
        if (rstatus[snb.dst] == level && 
            lstatus[snb.src] > level + 1) {
            lstatus[snb.src] = level + 1;
            ++frontier;
            //cout << " " << snb.src + src_offset << endl;
        }
    }
    return frontier;
}

template<class T>
void mem_bfs_snb(gview_t<T>* viewh,
        uint8_t* status, sid_t root)
{
    snap_t<T>* snaph = (snap_t<T>*)viewh;
	int		   top_down   = 1;
	sid_t	   frontier   = 0;
    sid_t      tile_count = snaph->get_vcount();
    sid_t      v_count    = _global_vcount;
    vid_t      p = (v_count >> bit_shift1) 
                 + (0 != (v_count & part_mask1_2));
    
    //uint8_t* status = (uint8_t*)calloc(v_count, sizeof(uint8_t));
    //memset(status, 255, v_count);

	double start1 = mywtime();
    int	  level  = 1;
	status[root] = level;
    
	do {
		frontier = 0;
		double start = mywtime();
		#pragma omp parallel reduction(+:frontier)
		{
            degree_t nebr_count = 0;
            header_t<T> header; 
            T dst;
            vid_t index = 0, m, n, offset;

            #pragma omp for nowait
            for (vid_t i = 0; i < p; ++i) {
                for (vid_t j = 0; j < p; ++j) {
                    offset = ((i*p + j) << bit_shift2); 
                    for (vid_t s_i = 0; s_i < p_p; s_i++) {
                        for (vid_t s_j = 0; s_j < p_p; s_j++) {
                            index = offset + ((s_i << bit_shift3) + s_j);
                            m = ((i << bit_shift3) + s_i) << bit_shift2;
                            n = ((j << bit_shift3) + s_j) << bit_shift2; 
                            frontier += bfs_tile(snaph, index, status+m, status+n, level); 
                        }
                    }
                }
            }
        }
        
		double end = mywtime();
	
		++level;
	} while (frontier);
		
    double end1 = mywtime();
    cout << "BFS Time = " << end1 - start1 << endl;
    // print_bfs_summary(status, level, v_count);
}

template<class T, class U>
inline void print_bfs_summary(T* status, U level, vid_t v_count)
{
    for (int l = 1; l < level; ++l) {
        vid_t vid_count = 0;
        #pragma omp parallel for reduction (+:vid_count) 
        for (vid_t v = 0; v < v_count; ++v) {
            if (status[v] == l) ++vid_count;
            //if (status[v] == l && l == 3) cout << v << endl;
        }
        cout << " Level = " << l << " count = " << vid_count << endl;
    }
}


bool compare_and_swap(vid_t &x, vid_t &old_val, vid_t &new_val) {
    return __sync_bool_compare_and_swap(reinterpret_cast<uint32_t*>(&x),
                                    reinterpret_cast<const uint32_t&>(old_val),
                                    reinterpret_cast<const uint32_t&>(new_val));
}

// Returns k pairs with largest values from list of key-value pairs
template<typename KeyT, typename ValT>
std::vector<std::pair<ValT, KeyT>> TopK(
    const std::vector<std::pair<KeyT, ValT>> &to_sort, size_t k) {
    std::vector<std::pair<ValT, KeyT>> top_k;
    ValT min_so_far = 0;
    for (auto kvp : to_sort) {
        if ((top_k.size() < k) || (kvp.second > min_so_far)) {
            top_k.push_back(std::make_pair(kvp.second, kvp.first));
            std::sort(top_k.begin(), top_k.end(),
                        std::greater<std::pair<ValT, KeyT>>());
            if (top_k.size() > k)
                top_k.resize(k);
            min_so_far = top_k.back().first;
        }
    }
  return top_k;
}

// Place nodes u and v in same component of lower component ID
void Link(vid_t u, vid_t v, vid_t* comp) {
  vid_t p1 = comp[u];
  vid_t p2 = comp[v];
  while (p1 != p2) {
    vid_t high = p1 > p2 ? p1 : p2;
    vid_t low = p1 + (p2 - high);
    vid_t p_high = comp[high];
    // Was already 'low' or succeeded in writing 'low'
    if ((p_high == low) ||
        (p_high == high && compare_and_swap(comp[high], high, low)))
      break;
    p1 = comp[comp[high]];
    p2 = comp[low];
  }
}

// Reduce depth of tree for each component to 1 by crawling up parents
void Compress(vid_t v_count, vid_t* comp) {
    #pragma omp parallel for schedule(dynamic, 16384)
    for (vid_t n = 0; n < v_count; n++) {
        while (comp[n] != comp[comp[n]]) {
            comp[n] = comp[comp[n]];
        }
    }
}

vid_t SampleFrequentElement(vid_t* comp, vid_t v_count, index_t num_samples = 1024) {
  std::unordered_map<vid_t, int> sample_counts(32);
  using kvp_type = std::unordered_map<vid_t, int>::value_type;
  // Sample elements from 'comp'
  std::mt19937 gen;
  std::uniform_int_distribution<vid_t> distribution(0, v_count - 1);
  for (vid_t i = 0; i < num_samples; i++) {
    vid_t n = distribution(gen);
    sample_counts[comp[n]]++;
  }
  // Find most frequent element in samples (estimate of most frequent overall)
  auto most_frequent = std::max_element(
    sample_counts.begin(), sample_counts.end(),
    [](const kvp_type& a, const kvp_type& b) { return a.second < b.second; });
  float frac_of_graph = static_cast<float>(most_frequent->second) / num_samples;
  std::cout
    << "Skipping largest intermediate component (ID: " << most_frequent->first
    << ", approx. " << static_cast<int>(frac_of_graph * 100)
    << "% of the graph)" << std::endl;
  return most_frequent->first;
}


void PrintCompStats(vid_t* comp, vid_t v_count) {
    using NodeID = vid_t;
    printf("\n");
    std::unordered_map<NodeID, NodeID> count;
    for(NodeID i=0; i<v_count; i++) {
        NodeID comp_i = comp[i];
        count[comp_i] += 1;
    }
    int k = 5;
    std::vector<std::pair<NodeID, NodeID>> count_vector;
    count_vector.reserve(count.size());
    for (auto kvp : count)
        count_vector.push_back(kvp);
    std::vector<std::pair<NodeID, NodeID>> top_k = TopK(count_vector, k);
    k = std::min(k, static_cast<int>(top_k.size()));
    printf("%d biggest clusters\n", k);
    for(auto kvp : top_k)
        printf("%d:%d\n", kvp.second, kvp.first);
    printf("There are %d components\n", count.size());
}

template<class T>
void cc_gapbs(gview_t<T>* snaph, index_t neighbor_rounds = 2, bool logging = false) {
    double st = mywtime();

    vid_t v_count = snaph->get_vcount();
    vid_t* comp = (vid_t*)calloc(sizeof(vid_t), v_count);
    

    // Initialize each node to a single-node self-pointing tree
    #pragma omp parallel for
    for (vid_t v = 0; v < v_count; v++) {
        comp[v] = v;
    }

    double t1 = mywtime();
    printf("Initialization time = %f\n", t1 - st);

    // Process a sparse sampled subgraph first for approximating components.
    // Sample by processing a fixed number of neighbors for each node (see paper)
    

    // for (index_t r = 0; r < neighbor_rounds; ++r) {
    size_t r = neighbor_rounds;
        #pragma omp parallel for schedule(dynamic, 16384)
        for (vid_t u = 0; u < v_count; u++) {
            degree_t      delta_degree = 0;
            degree_t        nebr_count = 0;
            degree_t      local_degree = 0;
            
            delta_adjlist_t<T>* delta_adjlist;
            T* local_adjlist = 0;

            degree_t d = 0;
            delta_adjlist = snaph->get_nebrs_archived_out(u);
            nebr_count = snaph->get_degree_out(u);

            if (0 != delta_adjlist && nebr_count != 0) {
                delta_degree = nebr_count;
                while (delta_adjlist != 0 && delta_degree > 0 && d <= r) {
                    local_adjlist = delta_adjlist->get_adjlist();
                    local_degree = delta_adjlist->get_nebrcount();
                    degree_t i_count = min(local_degree, delta_degree);
                    for (degree_t i = 0; i < i_count; ++i) {
                        if(d < r) {
                            sid_t v = get_sid(local_adjlist[i]);
                            // printf("Linking %d and %d\n", u, v);
                            Link(u, v, comp);
                            d++;
                        }
                        if(d == r) {
                            break;
                        }
                    }
                    delta_adjlist = delta_adjlist->get_next();
                    delta_degree -= local_degree;
                }
            }
        }
        Compress(v_count, comp);
    // }

    vid_t c = SampleFrequentElement(comp, v_count);
    double t2 = mywtime();
    printf("Sampling time = %f\n", t2 - t1);


    #pragma omp parallel for schedule(dynamic, 16384)
    for (vid_t u = 0; u < v_count; u++) {
        if (comp[u] == c) continue;

        degree_t      delta_degree = 0;
        degree_t        nebr_count = 0;
        degree_t      local_degree = 0;
        degree_t      d = 0;
        
        delta_adjlist_t<T>* delta_adjlist;
        T* local_adjlist = 0;

        delta_adjlist = snaph->get_nebrs_archived_out(u);
        nebr_count = snaph->get_degree_out(u);
        if (0 != delta_adjlist && nebr_count != 0) {
            delta_degree = nebr_count;
            while (delta_adjlist != 0 && delta_degree > 0) {
                local_adjlist = delta_adjlist->get_adjlist();
                local_degree = delta_adjlist->get_nebrcount();
                degree_t i_count = min(local_degree, delta_degree);
                for (degree_t i = 0; i < i_count; ++i) {
                    if(d > neighbor_rounds) {
                        vid_t v = get_sid(local_adjlist[i]);
                        Link(u, v, comp);
                    }
                    d++;
                }
                delta_adjlist = delta_adjlist->get_next();
                delta_degree -= local_degree;
            }
        }

        d = 0;
        // To support directed graphs, process reverse graph completely
        delta_adjlist = snaph->get_nebrs_archived_in(u);
        nebr_count = snaph->get_degree_in(u);
        if (0 != delta_adjlist && nebr_count != 0) {
            delta_degree = nebr_count;
            while (delta_adjlist != 0 && delta_degree > 0) {
                local_adjlist = delta_adjlist->get_adjlist();
                local_degree = delta_adjlist->get_nebrcount();
                degree_t i_count = min(local_degree, delta_degree);
                for (degree_t i = 0; i < i_count; ++i) {
                    if(d > neighbor_rounds) {
                        vid_t v = get_sid(local_adjlist[i]);
                        Link(u, v, comp);
                    }
                    d++;
                }
                delta_adjlist = delta_adjlist->get_next();
                delta_degree -= local_degree;
            }
        }
    }
    // Finally, 'compress' for final convergence
    Compress(v_count, comp);

    double t3 = mywtime();
    printf("Connected components time = %f\n", t3 - t2);

    if(logging)
        PrintCompStats(comp, v_count);
    double t4 = mywtime();
    printf("Logging time = %f\n", t4 - t3);
}


template<class T>
void pr_gapbs(gview_t<T>* snaph, int max_iters, double epsilon=0, bool logging_enabled = false)
{
    using NodeID = vid_t;
    using ScoreT = float;
    vid_t v_count = snaph->get_vcount();
    const float kDamp = 0.85;
    const ScoreT init_score = 1.0f / v_count;
    const ScoreT base_score = (1.0f - kDamp) / v_count;



	float* scores = 0 ;
	float* outgoing_contrib = 0;
	
    double start = mywtime();
    
    scores  = (float*)mmap(NULL, sizeof(float)*v_count, PROT_READ|PROT_WRITE,
                            MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB|MAP_HUGE_2MB, 0, 0 );
    if (MAP_FAILED == scores) {
        cout << "Huge page alloc failed for rank array" << endl;
        scores = (float*)calloc(v_count, sizeof(float));
    }
    
    outgoing_contrib  = (float*)mmap(NULL, sizeof(float)*v_count, PROT_READ|PROT_WRITE,
                            MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB|MAP_HUGE_2MB, 0, 0 );
    if (MAP_FAILED == outgoing_contrib) {
        cout << "Huge page alloc failed for prior rank array" << endl;
        outgoing_contrib = (float*)calloc(v_count, sizeof(float));
    }
    
    std::fill_n(scores, v_count, init_score);
	
    #pragma omp parallel for
    for (NodeID n=0; n < v_count; n++) {
        outgoing_contrib[n] = init_score / snaph->get_degree_out(n);
    }


	//let's run the pagerank
	for (int iter=0; iter < max_iters; iter++) {
        double start1 = mywtime();
        double error = 0;

        #pragma omp parallel 
        {
            sid_t sid;
            degree_t delta_degree = 0;
            degree_t nebr_count = 0;
            degree_t local_degree = 0;

            delta_adjlist_t<T>* delta_adjlist;
            T* local_adjlist = 0;
            
            #pragma omp for schedule (dynamic, 4096) reduction(+:error)
            for (vid_t u = 0; u < v_count; u++) {
                delta_adjlist = snaph->get_nebrs_archived_in(u);
                if (0 == delta_adjlist) continue;
                
                nebr_count = snaph->get_degree_in(u);
                float incoming_total = 0;
                
                //traverse the delta adj list
                delta_degree = nebr_count;
                while (delta_adjlist != 0 && delta_degree > 0) {
                    local_adjlist = delta_adjlist->get_adjlist();
                    local_degree = delta_adjlist->get_nebrcount();
                    degree_t i_count = min(local_degree, delta_degree);
                    for (degree_t i = 0; i < i_count; ++i) {
                        sid = get_sid(local_adjlist[i]);
                        incoming_total += outgoing_contrib[sid];
                    }
                    delta_adjlist = delta_adjlist->get_next();
                    delta_degree -= local_degree;
                }
                ScoreT old_score = scores[u];
                scores[u] = base_score + kDamp * incoming_total;
                error += fabs(scores[u] - old_score);
                outgoing_contrib[u] = scores[u] / snaph->get_degree_out(u);
            }
        }

        if(error < epsilon)
            break;
        double end1 = mywtime();
        cout << "Iteration Time = " << end1 - start1 << endl;
    }
    double end = mywtime();

	cout << "PR Time = " << end - start << endl;
	cout << endl;
}
