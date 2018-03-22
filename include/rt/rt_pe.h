/*
 * Copyright (C) 2018 ETH Zurich and University of Bologna
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Copyright (C) 2018 GreenWaves Technologies
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __RT_RT_PE_H__
#define __RT_RT_PE_H__

/**        
 * @ingroup groupCluster        
 */

/**        
 * @defgroup Team Team synchronization primitives
 *
 * Once a cluster entry point has been entered, after it has been enqueued by the fabric controller, all the following primitives can be used
 * to do multi-core processing, in a fork-join manner (like OMP parallel primitive).
 *
 * The execution can first be forked in order to activate more cores, and then synchronized together using barriers.
 */

/**        
 * @addtogroup Team
 * @{        
 */

/**@{*/

/** \brief Fork the execution of the calling core.
 *
 * Calling this function will create a team of workers and call the specified entry point
 * to start multi-core processing.
 * The team parameters (number of cores and stacks) are by default the ones configured when calling
 * the cluster entry point from the fabric controller. It is possible to use different parameters
 * when doing a new fork. If this is done the new parameters will become the new default ones.
 * If the number of cores is not provided (i.e. is zero), the number of cores of the previous
 * fork will be reused. Doing this has less runtime overhead.
 * 
 * \param        nb_cores The number of cores which will enter the entry point. If it is zero, this will reuse the number of cores of the previous fork or the default.
 * \param        entry The function entry point to be executed by all cores of the team.
 * \param        arg   The argument of the function entry point.
 */
static inline void rt_team_fork(int nb_cores, void (*entry)(void *), void *arg);

// TODO this will be soon implemented to reconfigure the stacks. 
// static inline void rt_team_fork_reconf(int nb_cores, void (*entry)(void *), void *arg, void *stacks);



/** \brief Execute a barrier between all cores of the team.
 *
 * This will block the execution of each calling core until all cores have reached the barrier.
 * The set of cores participating in the barrier is the one created with the last fork.
 * Each core of the team must execute the barrier exactly once for all cores to be able to go through the barrier.
 */
static inline void rt_team_barrier();

//!@}

/**        
 * @} end of Team group        
 */


/// @cond IMPLEM

#if defined(ARCHI_HAS_CLUSTER)

#if defined(EU_VERSION) && EU_VERSION >= 3

static inline void __rt_team_config(int nb_cores) {
  unsigned int core_mask = (1<<nb_cores) - 1;
  eu_dispatch_team_config(core_mask);
  eu_bar_setup(eu_bar_addr(0), core_mask);
}

static inline void rt_team_fork(int nb_cores, void (*entry)(void *), void *arg) {
  if (nb_cores) __rt_team_config(nb_cores);
  eu_dispatch_push((int)entry);
  eu_dispatch_push((int)arg);
  entry(arg);

  rt_team_barrier();
}

static inline void rt_team_barrier() {
  eu_bar_trig_wait_clr(eu_bar_addr(0));
}

#else

void __rt_team_fork(int nb_cores, void (*entry)(void *), void *arg);

void __rt_team_barrier();

static inline void rt_team_fork(int nb_cores, void (*entry)(void *), void *arg)
{
  __rt_team_fork(nb_cores, entry, arg);
}

static inline void rt_team_barrier()
{
  __rt_team_barrier();
}


#endif

#endif

/// @endcond

#endif
