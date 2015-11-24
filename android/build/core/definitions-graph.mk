# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Definitions of various graph-related generic functions, used by
# ndk-build internally.
#

# Coding style note:
#
# All internal variables in this file begin with '_build_mod_'
# All internal functions in this file begin with '-build-mod-'
#

# Set this to true if you want to debug the functions here.
_build_mod_debug := $(if $(BUILD_DEBUG_MODULES),true)
_build_topo_debug := $(if $(BUILD_DEBUG_TOPO),true)

# Use $(call -build-mod-debug,<message>) to print a debug message only
# if _build_mod_debug is set to 'true'. Useful for debugging the functions
# available here.
#
ifeq (true,$(_build_mod_debug))
-build-mod-debug = $(info $1)
else
-build-mod-debug := $(empty)
endif

ifeq (true,$(_build_topo_debug))
-build-topo-debug = $(info $1)
else
-build-topo-debug = $(empty)
endif

#######################################################################
# Filter a list of module with a predicate function
# $1: list of module names.
# $2: predicate function, will be called with $(call $2,<name>), if the
#     result is not empty, <name> will be added to the result.
# Out: subset of input list, where each item passes the predicate.
#######################################################################
-build-mod-filter = $(strip \
    $(foreach _build_mod_filter_n,$1,\
        $(if $(call $2,$(_build_mod_filter_n)),$(_build_mod_filter_n))\
    ))

-test-build-mod-filter = \
    $(eval -local-func = $$(call seq,foo,$$1))\
    $(call test-expect,,$(call -build-mod-filter,,-local-func))\
    $(call test-expect,foo,$(call -build-mod-filter,foo,-local-func))\
    $(call test-expect,foo,$(call -build-mod-filter,foo bar,-local-func))\
    $(call test-expect,foo foo,$(call -build-mod-filter,aaa foo bar foo,-local-func))\
    $(eval -local-func = $$(call sne,foo,$$1))\
    $(call test-expect,,$(call -build-mod-filter,,-local-func))\
    $(call test-expect,,$(call -build-mod-filter,foo,-local-func))\
    $(call test-expect,bar,$(call -build-mod-filter,foo bar,-local-func))\
    $(call test-expect,aaa bar,$(call -build-mod-filter,aaa foo bar,-local-func))


#######################################################################
# Filter out a list of modules with a predicate function
# $1: list of module names.
# $2: predicate function, will be called with $(call $2,<name>), if the
#     result is not empty, <name> will be added to the result.
# Out: subset of input list, where each item doesn't pass the predicate.
#######################################################################
-build-mod-filter-out = $(strip \
    $(foreach _build_mod_filter_n,$1,\
        $(if $(call $2,$(_build_mod_filter_n)),,$(_build_mod_filter_n))\
    ))

-test-build-mod-filter-out = \
    $(eval -local-func = $$(call seq,foo,$$1))\
    $(call test-expect,,$(call -build-mod-filter-out,,-local-func))\
    $(call test-expect,,$(call -build-mod-filter-out,foo,-local-func))\
    $(call test-expect,bar,$(call -build-mod-filter-out,foo bar,-local-func))\
    $(call test-expect,aaa bar,$(call -build-mod-filter-out,aaa foo bar foo,-local-func))\
    $(eval -local-func = $$(call sne,foo,$$1))\
    $(call test-expect,,$(call -build-mod-filter-out,,-local-func))\
    $(call test-expect,foo,$(call -build-mod-filter-out,foo,-local-func))\
    $(call test-expect,foo,$(call -build-mod-filter-out,foo bar,-local-func))\
    $(call test-expect,foo foo,$(call -build-mod-filter-out,aaa foo bar foo,-local-func))


#######################################################################
# Find the first item in a list that checks a valid predicate.
# $1: list of names.
# $2: predicate function, will be called with $(call $2,<name>), if the
#     result is not empty, <name> will be added to the result.
# Out: subset of input list.
#######################################################################
-build-mod-find-first = $(firstword $(call -build-mod-filter,$1,$2))

-test-build-mod-find-first.empty = \
    $(eval -local-pred = $$(call seq,foo,$$1))\
    $(call test-expect,,$(call -build-mod-find-first,,-local-pred))\
    $(call test-expect,,$(call -build-mod-find-first,bar,-local-pred))

-test-build-mod-find-first.simple = \
    $(eval -local-pred = $$(call seq,foo,$$1))\
    $(call test-expect,foo,$(call -build-mod-find-first,foo,-local-pred))\
    $(call test-expect,foo,$(call -build-mod-find-first,aaa foo bar,-local-pred))\
    $(call test-expect,foo,$(call -build-mod-find-first,aaa foo foo bar,-local-pred))

########################################################################
# Many tree walking operations require setting a 'visited' flag on
# specific graph nodes. The following helper functions help implement
# this while hiding details to the callers.
#
# Technical note:
#  _build_mod_tree_visited.<name> will be 'true' if the node was visited,
#  or empty otherwise.
#
#  _build_mod_tree_visitors lists all visited nodes, used to clean all
#  _build_mod_tree_visited.<name> variables in -build-mod-tree-setup-visit.
#
#######################################################################

# Call this before tree traversal.
-build-mod-tree-setup-visit = \
    $(foreach _build_mod_tree_visitor,$(_build_mod_tree_visitors),\
        $(eval _build_mod_tree_visited.$$(_build_mod_tree_visitor) :=))\
    $(eval _build_mod_tree_visitors :=)

# Returns non-empty if a node was visited.
-build-mod-tree-is-visited = \
    $(_build_mod_tree_visited.$1)

# Set the visited state of a node to 'true'
-build-mod-tree-set-visited = \
    $(eval _build_mod_tree_visited.$1 := true)\
    $(eval _build_mod_tree_visitors += $1)

########################################################################
# Many graph walking operations require a work queue and computing
# dependencies / children nodes. Here are a few helper functions that
# can be used to make their code clearer. This uses a few global
# variables that should be defined as follows during the operation:
#
#  _build_mod_module     current graph node name.
#  _build_mod_wq         current node work queue.
#  _build_mod_list       current result (list of nodes).
#  _build_mod_depends    current graph node's children.
#                      you must call -build-mod-get-depends to set this.
#
#######################################################################

# Pop first item from work-queue into _build_mod_module.
-build-mod-pop-first = \
    $(eval _build_mod_module := $$(call first,$$(_build_mod_wq)))\
    $(eval _build_mod_wq     := $$(call rest,$$(_build_mod_wq)))

-test-build-mod-pop-first = \
    $(eval _build_mod_wq := A B C)\
    $(call -build-mod-pop-first)\
    $(call test-expect,A,$(_build_mod_module))\
    $(call test-expect,B C,$(_build_mod_wq))\


# Push list of items at the back of the work-queue.
-build-mod-push-back = \
    $(eval _build_mod_wq := $(strip $(_build_mod_wq) $1))

-test-build-mod-push-back = \
  $(eval _build_mod_wq := A B C)\
  $(call -build-mod-push-back, D    E)\
  $(call test-expect,A B C D E,$(_build_mod_wq))

# Set _build_mod_depends to the direct dependencies of _build_mod_module
-build-mod-get-depends = \
    $(eval _build_mod_depends := $$(call $$(_build_mod_deps_func),$$(_build_mod_module)))

# Set _build_mod_depends to the direct dependencies of _build_mod_module that
# are not already in _build_mod_list.
-build-mod-get-new-depends = \
    $(call -build-mod-get-depends)\
    $(eval _build_mod_depends := $$(filter-out $$(_build_mod_list),$$(_build_mod_depends)))

##########################################################################
# Compute the transitive closure
# $1: list of modules.
# $2: dependency function, $(call $2,<module>) should return all the
#     module that <module> depends on.
# Out: transitive closure of all modules from those in $1. Always includes
#      the modules in $1. Order is random.
#
# Implementation note:
#   we use the -build-mod-tree-xxx functions to flag 'visited' nodes
#   in the graph. A node is visited once it has been put into the work
#   queue. For each item in the work queue, get the dependencies and
#   append all those that were not visited yet.
#######################################################################
-build-mod-get-closure = $(strip \
    $(eval _build_mod_wq :=)\
    $(eval _build_mod_list :=)\
    $(eval _build_mod_deps_func := $2)\
    $(call -build-mod-tree-setup-visit)\
    $(foreach _build_mod_module,$1,\
        $(call -build-mod-closure-visit,$(_build_mod_module))\
    )\
    $(call -build-mod-closure-recursive)\
    $(eval _build_mod_deps :=)\
    $(_build_mod_list)\
    )

# Used internally to visit a new node during -build-mod-get-closure.
# This appends the node to the work queue, and set its 'visit' flag.
-build-mod-closure-visit = \
    $(call -build-mod-push-back,$1)\
    $(call -build-mod-tree-set-visited,$1)

-build-mod-closure-recursive = \
    $(call -build-mod-pop-first)\
    $(eval _build_mod_list += $$(_build_mod_module))\
    $(call -build-mod-get-depends)\
    $(foreach _build_mod_dep,$(_build_mod_depends),\
        $(if $(call -build-mod-tree-is-visited,$(_build_mod_dep)),,\
        $(call -build-mod-closure-visit,$(_build_mod_dep))\
        )\
    )\
    $(if $(_build_mod_wq),$(call -build-mod-closure-recursive))

-test-build-mod-get-closure.empty = \
    $(eval -local-deps = $$($$1_depends))\
    $(call test-expect,,$(call -build-mod-get-closure,,-local-deps))

-test-build-mod-get-closure.single = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends :=)\
    $(call test-expect,A,$(call -build-mod-get-closure,A,-local-deps))

-test-build-mod-get-closure.double = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends := B)\
    $(eval B_depends :=)\
    $(call test-expect,A B,$(call -build-mod-get-closure,A,-local-deps))

-test-build-mod-get-closure.circular-deps = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends := B)\
    $(eval B_depends := C)\
    $(eval C_depends := A)\
    $(call test-expect,A B C,$(call -build-mod-get-closure,A,-local-deps))

-test-build-mod-get-closure.ABCDE = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends := B C)\
    $(eval B_depends := D)\
    $(eval C_depends := D E)\
    $(eval D_depends :=)\
    $(eval E_depends :=)\
    $(call test-expect,A B C D E,$(call -build-mod-get-closure,A,-local-deps))


#########################################################################
# For topological sort, we need to count the number of incoming edges
# in each graph node. The following helper functions implement this and
# hide implementation details.
#
# Count the number of incoming edges for each node during topological
# sort with a string of xxxxs. I.e.:
#  0 edge  -> ''
#  1 edge  -> 'x'
#  2 edges -> 'xx'
#  3 edges -> 'xxx'
#  etc.
#########################################################################

# zero the incoming edge counter for module $1
-build-mod-topo-zero-incoming = \
    $(eval _build_mod_topo_incoming.$1 :=)

# increment the incoming edge counter for module $1
-build-mod-topo-increment-incoming = \
    $(eval _build_mod_topo_incoming.$1 := $$(_build_mod_topo_incoming.$1)x)

# decrement the incoming edge counter for module $1
-build-mod-topo-decrement-incoming = \
    $(eval _build_mod_topo_incoming.$1 := $$(_build_mod_topo_incoming.$1:%x=%))

# return non-empty if the module $1's incoming edge counter is > 0
-build-mod-topo-has-incoming = $(_build_mod_topo_incoming.$1)

# Find first node in a list that has zero incoming edges.
# $1: list of nodes
# Out: first node that has zero incoming edges, or empty.
-build-mod-topo-find-first-zero-incoming = $(firstword $(call -build-mod-filter-out,$1,-build-mod-topo-has-incoming))

# Only use for debugging:
-build-mod-topo-dump-count = \
    $(foreach _build_mod_module,$1,\
        $(info .. $(_build_mod_module) incoming='$(_build_mod_topo_incoming.$(_build_mod_module))'))



#########################################################################
# Return the topologically ordered closure of all nodes from a top-level
# one. This means that a node A, in the result, will always appear after
# node B if A depends on B. Assumes that the graph is a DAG (if there are
# circular dependencies, this property cannot be guaranteed, but at least
# the function should not loop infinitely).
#
# $1: top-level node name.
# $2: dependency function, i.e. $(call $2,<name>) returns the children
#     nodes for <name>.
# Return: list of nodes, include $1, which will always be the first.
#########################################################################
-build-mod-get-topo-list = $(strip \
    $(eval _build_mod_top_module := $1)\
    $(eval _build_mod_deps_func := $2)\
    $(eval _build_mod_nodes := $(call -build-mod-get-closure,$1,$2))\
    $(call -build-mod-topo-count,$(_build_mod_nodes))\
    $(eval _build_mod_list :=)\
    $(eval _build_mod_wq := $(call -build-mod-topo-find-first-zero-incoming,$(_build_mod_nodes)))\
    $(call -build-mod-topo-sort)\
    $(_build_mod_list) $(_build_mod_nodes)\
    )

# Given a closure list of nodes, count their incoming edges.
# $1: list of nodes, must be a graph closure.
-build-mod-topo-count = \
    $(foreach _build_mod_module,$1,\
        $(call -build-mod-topo-zero-incoming,$(_build_mod_module)))\
    $(foreach _build_mod_module,$1,\
        $(call -build-mod-get-depends)\
        $(foreach _build_mod_dep,$(_build_mod_depends),\
        $(call -build-mod-topo-increment-incoming,$(_build_mod_dep))\
        )\
    )

-build-mod-topo-sort = \
    $(call -build-topo-debug,-build-mod-topo-sort: wq='$(_build_mod_wq)' list='$(_build_mod_list)')\
    $(call -build-mod-pop-first)\
    $(if $(_build_mod_module),\
        $(eval _build_mod_list += $(_build_mod_module))\
        $(eval _build_mod_nodes := $(filter-out $(_build_mod_module),$(_build_mod_nodes)))\
        $(call -build-mod-topo-decrement-incoming,$(_build_mod_module))\
        $(call -build-mod-get-depends)\
        $(call -build-topo-debug,-build-mod-topo-sort:   deps='$(_build_mod_depends)')\
        $(foreach _build_mod_dep,$(_build_mod_depends),\
            $(call -build-mod-topo-decrement-incoming,$(_build_mod_dep))\
            $(if $(call -build-mod-topo-has-incoming,$(_build_mod_dep)),,\
                $(call -build-mod-push-back,$(_build_mod_dep))\
            )\
        )\
        $(call -build-mod-topo-sort)\
    )


-test-build-mod-get-topo-list.empty = \
    $(eval -local-deps = $$($$1_depends))\
    $(call test-expect,,$(call -build-mod-get-topo-list,,-local-deps))

-test-build-mod-get-topo-list.single = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends :=)\
    $(call test-expect,A,$(call -build-mod-get-topo-list,A,-local-deps))

-test-build-mod-get-topo-list.no-infinite-loop = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends := B)\
    $(eval B_depends := C)\
    $(eval C_depends := A)\
    $(call test-expect,A B C,$(call -build-mod-get-topo-list,A,-local-deps))

-test-build-mod-get-topo-list.ABC = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends := B C)\
    $(eval B_depends :=)\
    $(eval C_depends := B)\
    $(call test-expect,A C B,$(call -build-mod-get-topo-list,A,-local-deps))

-test-build-mod-get-topo-list.ABCD = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends := B C)\
    $(eval B_depends := D)\
    $(eval C_depends := B)\
    $(eval D_depends :=)\
    $(call test-expect,A C B D,$(call -build-mod-get-topo-list,A,-local-deps))

-test-build-mod-get-topo-list.ABC.circular = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends := B)\
    $(eval B_depends := C)\
    $(eval C_depends := B)\
    $(call test-expect,A B C,$(call -build-mod-get-topo-list,A,-local-deps))

#########################################################################
# Return the topologically ordered closure of all dependencies from a
# top-level node.
#
# $1: top-level node name.
# $2: dependency function, i.e. $(call $2,<name>) returns the children
#     nodes for <name>.
# Return: list of nodes, include $1, which will never be included.
#########################################################################
-build-mod-get-topological-depends = $(call rest,$(call -build-mod-get-topo-list,$1,$2))

-test-build-mod-get-topological-depends.simple = \
    $(eval -local-get-deps = $$($$1_depends))\
    $(eval A_depends := B)\
    $(eval B_depends :=)\
    $(eval topo_deps := $$(call -build-mod-get-topological-depends,A,-local-get-deps))\
    $(call test-expect,B,$(topo_deps),topo dependencies)

-test-build-mod-get-topological-depends.ABC = \
    $(eval -local-get-deps = $$($$1_depends))\
    $(eval A_depends := B C)\
    $(eval B_depends :=)\
    $(eval C_depends := B)\
    $(eval bfs_deps := $$(call -build-mod-get-bfs-depends,A,-local-get-deps))\
    $(eval topo_deps := $$(call -build-mod-get-topological-depends,A,-local-get-deps))\
    $(call test-expect,B C,$(bfs_deps),dfs dependencies)\
    $(call test-expect,C B,$(topo_deps),topo dependencies)

-test-build-mod-get-topological-depends.circular = \
    $(eval -local-get-deps = $$($$1_depends))\
    $(eval A_depends := B)\
    $(eval B_depends := C)\
    $(eval C_depends := B)\
    $(eval bfs_deps := $$(call -build-mod-get-bfs-depends,A,-local-get-deps))\
    $(eval topo_deps := $$(call -build-mod-get-topological-depends,A,-local-get-deps))\
    $(call test-expect,B C,$(bfs_deps),dfs dependencies)\
    $(call test-expect,B C,$(topo_deps),topo dependencies)

#########################################################################
# Return breadth-first walk of a graph, starting from an arbitrary
# node.
#
# This performs a breadth-first walk of the graph and will return a
# list of nodes. Note that $1 will always be the first in the list.
#
# $1: root node name.
# $2: dependency function, i.e. $(call $2,<name>) returns the nodes
#     that <name> depends on.
# Result: list of dependent modules, $1 will be part of it.
#########################################################################
-build-mod-get-bfs-list = $(strip \
    $(eval _build_mod_wq := $(call strip-lib-prefix,$1)) \
    $(eval _build_mod_deps_func := $2)\
    $(eval _build_mod_list :=)\
    $(call -build-mod-tree-setup-visit)\
    $(call -build-mod-tree-set-visited,$(_build_mod_wq))\
    $(call -build-mod-bfs-recursive) \
    $(_build_mod_list))

# Recursive function used to perform a depth-first scan.
# Must initialize _build_mod_list, _build_mod_field, _build_mod_wq
# before calling this.
-build-mod-bfs-recursive = \
    $(call -build-mod-debug,-build-mod-bfs-recursive wq='$(_build_mod_wq)' list='$(_build_mod_list)' visited='$(_build_mod_tree_visitors)')\
    $(call -build-mod-pop-first)\
    $(eval _build_mod_list += $$(_build_mod_module))\
    $(call -build-mod-get-depends)\
    $(call -build-mod-debug,.  node='$(_build_mod_module)' deps='$(_build_mod_depends)')\
    $(foreach _build_mod_child,$(_build_mod_depends),\
        $(if $(call -build-mod-tree-is-visited,$(_build_mod_child)),,\
            $(call -build-mod-tree-set-visited,$(_build_mod_child))\
            $(call -build-mod-push-back,$(_build_mod_child))\
        )\
    )\
    $(if $(_build_mod_wq),$(call -build-mod-bfs-recursive))

-test-build-mod-get-bfs-list.empty = \
    $(eval -local-deps = $$($$1_depends))\
    $(call test-expect,,$(call -build-mod-get-bfs-list,,-local-deps))

-test-build-mod-get-bfs-list.A = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends :=)\
    $(call test-expect,A,$(call -build-mod-get-bfs-list,A,-local-deps))

-test-build-mod-get-bfs-list.ABCDEF = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends := B C)\
    $(eval B_depends := D E)\
    $(eval C_depends := F E)\
    $(eval D_depends :=)\
    $(eval E_depends :=)\
    $(eval F_depends :=)\
    $(call test-expect,A B C D E F,$(call -build-mod-get-bfs-list,A,-local-deps))

#########################################################################
# Return breadth-first walk of a graph, starting from an arbitrary
# node.
#
# This performs a breadth-first walk of the graph and will return a
# list of nodes. Note that $1 will _not_ be part of the list.
#
# $1: root node name.
# $2: dependency function, i.e. $(call $2,<name>) returns the nodes
#     that <name> depends on.
# Result: list of dependent modules, $1 will not be part of it.
#########################################################################
-build-mod-get-bfs-depends = $(call rest,$(call -build-mod-get-bfs-list,$1,$2))

-test-build-mod-get-bfs-depends.simple = \
    $(eval -local-deps-func = $$($$1_depends))\
    $(eval A_depends := B)\
    $(eval B_depends :=)\
    $(eval deps := $$(call -build-mod-get-bfs-depends,A,-local-deps-func))\
    $(call test-expect,B,$(deps))

-test-build-mod-get-bfs-depends.ABC = \
    $(eval -local-deps-func = $$($$1_depends))\
    $(eval A_depends := B C)\
    $(eval B_depends :=)\
    $(eval C_depends := B)\
    $(eval deps := $$(call -build-mod-get-bfs-depends,A,-local-deps-func))\
    $(call test-expect,B C,$(deps))\

-test-build-mod-get-bfs-depends.ABCDE = \
    $(eval -local-deps-func = $$($$1_depends))\
    $(eval A_depends := B C)\
    $(eval B_depends := D)\
    $(eval C_depends := D E F)\
    $(eval D_depends :=)\
    $(eval E_depends :=)\
    $(eval F_depends :=)\
    $(eval deps := $$(call -build-mod-get-bfs-depends,A,-local-deps-func))\
    $(call test-expect,B C D E F,$(deps))\

-test-build-mod-get-bfs-depends.loop = \
    $(eval -local-deps-func = $$($$1_depends))\
    $(eval A_depends := B)\
    $(eval B_depends := A)\
    $(eval deps := $$(call -build-mod-get-bfs-depends,A,-local-deps-func))\
    $(call test-expect,B,$(deps))
