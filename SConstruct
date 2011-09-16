#!/bin/env scons
#===============================================================================
#
# Main SCons script for SIT release building
#
# $Id: SConstruct.main 2233 2011-08-17 23:52:36Z salnikov@SLAC.STANFORD.EDU $
#
#===============================================================================

import os
import sys
from pprint import *
from os.path import join as pjoin

cwd = os.getcwd()

#   check that SIT_ARCH is defined
sit_arch=os.environ.get( "SIT_ARCH", None )
if not sit_arch :
    print >> sys.stderr, "Environment variable SIT_ARCH is not defined"
    Exit(2)
sit_root=os.environ.get( "SIT_ROOT", None )
if not sit_root :
    print >> sys.stderr, "Environment variable SIT_ROOT is not defined"
    Exit(2)

# check .sit_release
try:
    test_rel = file('.sit_release').read().strip()
except IOError:
    print >> sys.stderr, "File .sit_release does not exist or unreadable."
    print >> sys.stderr, "Trying to run scons outside release directory?"
    Exit(2)

sit_rel = os.environ.get( "SIT_RELEASE", None )
if not sit_rel :
    print >> sys.stderr, "Environment variable SIT_RELEASE is not defined"
    Exit(2)

if sit_rel != test_rel:
    print >> sys.stderr, "* SIT_RELEASE conflicts with release directory"
    print >> sys.stderr, "* SIT_RELEASE =", sit_rel
    print >> sys.stderr, "* .sit_release =", test_rel
    print >> sys.stderr, "* Please run sit_setup or relupgrade"
    Exit(2)

#
# Before doing any other imports link the python files from
# SConsTools/src/*.py to arch/$SIT_ARCH/python/SConsTools/
#
if os.path.isdir("SConsTools/src") :
    
    # list of python files in SConsTools/src
    pys = set ( [ f for f in os.listdir("SConsTools/src") if os.path.splitext(f)[1] == ".py" ] )
    
    # list of links in arch/$SIT_ARCH/python/SConsTools
    d = pjoin("arch",sit_arch,"python/SConsTools")
    if not os.path.isdir(d) : os.makedirs(d) 
    links = set ( [ f for f in os.listdir(d) if os.path.splitext(f)[1] == ".py" ] )
    
    # remove extra links
    for f in links - pys :
        try :
            os.remove( pjoin(d,f) )
        except Exception, e :
            print >> sys.stderr, "Failed to remove file "+pjoin(d,f)
            print >> sys.stderr, str(e)
            print >> sys.stderr, "Check your permissions and AFS token"
            Exit(2)
    
    # add missing links
    for f in pys - links :
        os.symlink ( pjoin(cwd,"SConsTools/src",f), pjoin(d,f) )

    init = pjoin(d,"__init__.py")
    if not os.path.isfile(init) :
        f = open( init, 'w' )
        f.close()
        del f
    del init


#
# Now can import rest of the stuff
#
from SConsTools.trace import *
from SConsTools.scons_functions import *
from SConsTools.scons_env import buildEnv
from SConsTools.compilers import setupCompilers
from SConsTools.builders import setupBuilders
from SConsTools.standardSConscript import standardSConscript
from SConsTools.dependencies import *

# ===================================
#   Setup default build environment
# ===================================
env = buildEnv()
setupBuilders( env )
setupCompilers( env )

#
# find out which packages we have locally
#
packages = [ d for d in os.listdir(cwd) if os.path.isfile(pjoin( d, "SConscript" )) ]
trace ( "Packages: " + pformat( packages ), "<top>", 1 )

#
# Check the links in include/, data/, web/
#
makePackageLinks ( "include", packages )
makePackageLinks ( "data", packages )
makePackageLinks ( "web", packages )

#
# load package dependencies from base releases
#
for r in reversed(env['SIT_REPOS']) :
    fname = pjoin ( r, env['PKG_DEPS_FILE'] )
    if os.path.isfile ( fname ) :
        loadPkgDeps ( env, fname )


#
# include all SConscript files from all packages
#
for p in packages :
    scons = pjoin(p,"SConscript")
    build = pjoin("#build",env['SIT_ARCH'],p)
    env.SConscript( pjoin(p,"SConscript"), 
                variant_dir=build,
                src_dir='#'+p, 
                duplicate=0,
                exports="env trace standardSConscript" )

#
# Analyze whole dependency tree and adjust dependencies and libraries
#
adjustPkgDeps ( env )

#
# Now store the dependencies in case somebody else would want to use them later
#
storePkgDeps ( env, env['PKG_DEPS_FILE'] )

#
# define few aliases and default targets
#
libs = env.Alias ( 'lib', env['ALL_TARGETS']['LIBS'] )
bins = env.Alias ( 'bin', env['ALL_TARGETS']['BINS'] )
all = env.Alias ( 'all', libs+bins )
tests = env.Alias ( 'test', env['ALL_TARGETS']['TESTS'] )
env.Requires ( env['ALL_TARGETS']['TESTS'], all )
env.Default ( all )

#
# Additional targets for documentation generation
#
docTargets = []
if 'psddl' in packages:
    docTargets.append(env.Command('doc/psddl_psana/index.html', env.Dir('psddl'), "ddl_psanadoc doc/psddl_psana/"))
if 'psana' in packages:
    docTargets.append(env.Command('doc/psana-doxy/html/index.html', env.Dir('psana'), "doxy-driver -m psana doc/psana-doxy/"))
if 'psana' in packages:
    docTargets.append(env.Command('doc/psana-modules-doxy/html/index.html', env.Dir('psana'), "doxy-driver -m psana-modules doc/psana-modules-doxy/"))
env.Alias ( 'doc', docTargets )

#trace ( "Build env = "+pformat(env.Dictionary()), "<top>", 7 )
trace ( "BUILD_TARGETS is " + pformat( map(str, BUILD_TARGETS) ), "<top>", 1 )
trace ( "DEFAULT_TARGETS is " + pformat( map(str, DEFAULT_TARGETS) ), "<top>", 1 )
trace ( "COMMAND_LINE_TARGETS is " + pformat( map(str, COMMAND_LINE_TARGETS) ), "<top>", 1 )
