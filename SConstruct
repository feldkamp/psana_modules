#!/bin/env scons
#===============================================================================
#
# Main SCons script for SIT release building
#
# $Id: SConstruct.main 4190 2012-07-24 16:56:30Z salnikov@SLAC.STANFORD.EDU $
#
#===============================================================================

import os
import sys
from pprint import *
from os.path import join as pjoin

cwd = os.getcwd()

#   check that all required envvars are defined
for var in ["SIT_ARCH", "SIT_ROOT", "SIT_RELEASE"]:
    if not os.environ.get(var, None):
        print >> sys.stderr, "Environment variable %s is not defined" % var
        Exit(2)

sit_arch = os.environ["SIT_ARCH"]

# check .sit_release
try:
    test_rel = file('.sit_release').read().strip()
except IOError:
    print >> sys.stderr, "File .sit_release does not exist or unreadable."
    print >> sys.stderr, "Trying to run scons outside release directory?"
    Exit(2)
if os.environ["SIT_RELEASE"] != test_rel:
    print >> sys.stderr, "* SIT_RELEASE conflicts with release directory"
    print >> sys.stderr, "* SIT_RELEASE =", os.environ["SIT_RELEASE"]
    print >> sys.stderr, "* .sit_release =", test_rel
    print >> sys.stderr, "* Please run sit_setup or relupgrade"
    Exit(2)

#
# Before doing any other imports link the python files from
# SConsTools/src/*.py and SConsTools/src/tools/*.py to arch/$SIT_ARCH/python/SConsTools/...
#
for dirs in [("SConsTools/src", "SConsTools"), ("SConsTools/src/tools", "SConsTools/tools")]:
    if os.path.isdir(dirs[0]):

        # python tends to remember empty directories, reset its cache
        sys.path_importer_cache = {}

        # list of python files in source directory
        pys = set(f for f in os.listdir(dirs[0]) if os.path.splitext(f)[1] == ".py")

        # list of links in arch/$SIT_ARCH/python/SConsTools
        d = pjoin("arch", sit_arch, "python", dirs[1])
        if not os.path.isdir(d): os.makedirs(d)
        links = set(f for f in os.listdir(d) if os.path.splitext(f)[1] == ".py")

        # remove extra links
        for f in links - pys:
            try:
                os.remove(pjoin(d, f))
            except Exception, e:
                print >> sys.stderr, "Failed to remove file " + pjoin(d, f)
                print >> sys.stderr, str(e)
                print >> sys.stderr, "Check your permissions and AFS token"
                Exit(2)

        # add missing links (make links relative)
        for f in pys - links:
            reldst = '/'.join(['..'] * len(d.split('/')))
            os.symlink(pjoin(reldst, dirs[0], f), pjoin(d, f))

        init = pjoin(d, "__init__.py")
        if not os.path.isfile(init):
            open(init, 'w').close()
        del init


#
# Now can import rest of the stuff
#
from SConsTools.trace import *
from SConsTools.scons_functions import *
from SConsTools.scons_env import buildEnv
from SConsTools.standardSConscript import standardSConscript
from SConsTools.dependencies import *

# ===================================
#   Setup default build environment
# ===================================
env = buildEnv()

# re-build dependencies based on timestamps
env.Decider('timestamp-newer')

# if help is requested then do not load any packages
if env.GetOption('help'): Return()

#
# Special install target
#
destdir = env['DESTDIR']
install = env.ReleaseInstall([Dir(destdir)], [])
env.AlwaysBuild(install)
env.Alias('install', [install])

#
# find out which packages we have locally
#
packages = [ d for d in os.listdir(cwd) if os.path.isfile(pjoin(d, "SConscript")) ]
trace("Packages: " + pformat(packages), "<top>", 1)

#
# Check the links in include/, data/, web/
#
makePackageLinks("include", packages)
makePackageLinks("data", packages)
makePackageLinks("web", packages)

#
# load package dependencies from base releases
#
trace("Loading existing package dependencies", "<top>", 1)
for r in reversed(env['SIT_REPOS']):
    fname = pjoin(r, env['PKG_DEPS_FILE'])
    if os.path.isfile(fname):
        loadPkgDeps(fname)


#
# include all SConscript files from all packages
#
trace("Reading packages SConscript files", "<top>", 1)
for p in packages:
    scons = pjoin(p, "SConscript")
    build = pjoin("#build", sit_arch, p)
    env.SConscript(pjoin(p, "SConscript"),
                variant_dir=build,
                src_dir='#' + p,
                duplicate=0,
                exports="env trace standardSConscript")

#
# Analyze whole dependency tree and adjust dependencies and libraries
#
trace("Recalculating packages dependencies", "<top>", 1)
adjustPkgDeps()

#
# Now store the dependencies in case somebody else would want to use them later
#
trace("Storing packages dependencies", "<top>", 1)
storePkgDeps(env['PKG_DEPS_FILE'])

#
# define few aliases and default targets
#
incs = env.Alias('includes', env['ALL_TARGETS']['INCLUDES'])
libs = env.Alias('lib', env['ALL_TARGETS']['LIBS'])
bins = env.Alias('bin', env['ALL_TARGETS']['BINS'])
all = env.Alias('all', incs + libs + bins)
tests = env.Alias('test', env['ALL_TARGETS']['TESTS'])
# these are not strictly necessary, just to make it look more make-ish
env.Requires(env['ALL_TARGETS']['TESTS'], all)
env.Requires(libs, incs)
env.Requires(bins, libs)
# default is to build all
env.Default(all)

#
# Special install target
#
destdir = env['DESTDIR']
install = env.ReleaseInstall([Dir(destdir)], [])
env.AlwaysBuild(install)
env.Alias('install', [install])
env.Requires(install, all)

#
# Special target for package list file
#
#pkg_list = env['PKG_LIST_FILE']
pkg_list = env.PackageList(['SConsTools.pkg_list'], [])
env.AlwaysBuild(pkg_list)
#env.Alias('SConsTools.pkg_list', pkg_list)

env.Command(['package-dependencies'], [], PrintDependencies([env['PKG_TREE_BASE'], env['PKG_TREE']]))
env.Command(['package-dependencies-reverse'], [], PrintDependencies([env['PKG_TREE_BASE'], env['PKG_TREE']], True))
env.Command(['package-dependencies-base'], [], PrintDependencies([env['PKG_TREE_BASE']]))
env.Command(['package-dependencies-local'], [], PrintDependencies([env['PKG_TREE']]))
env.AlwaysBuild('package-dependencies')
env.AlwaysBuild('package-dependencies-base')
env.AlwaysBuild('package-dependencies-local')
env.AlwaysBuild('package-dependencies-reverse')

#
# Additional targets for documentation generation
#
docTargets = []
if 'doctools' in packages:
    docTargets.append(env.Command('doc/doxy-all/html/index.html', env.Dir('doctools'), "doxy-driver -p 'PSDM Software' doc/doxy-all/ doctools/doc/mainpage.dox-main " +' '.join(packages)))
if 'psddl' in packages:
    docTargets.append(env.Command('doc/psddl_psana/index.html', env.Dir('psddl'), "ddl_psanadoc doc/psddl_psana/"))
if 'psana' in packages:
    docTargets.append(env.Command('doc/psana-doxy/html/index.html', env.Dir('psana'), "doxy-driver -m psana doc/psana-doxy/"))
    docTargets.append(env.Command('doc/psana-modules-doxy/html/index.html', env.Dir('psana'), "doxy-driver -m psana-modules doc/psana-modules-doxy/"))
if 'pyana' in packages:
    docTargets.append(env.Command('doc/pyana-ref/html/index.html', env.Dir('pyana'), "pydoc-driver -p pyana -Q doc/pyana-ref/html pyana _pdsdata pypdsdata"))
env.Alias('doc', docTargets)

#trace( "Build env = "+pformat(env.Dictionary()), "<top>", 7 )
trace("BUILD_TARGETS is " + pformat(map(str, BUILD_TARGETS)), "<top>", 1)
trace("DEFAULT_TARGETS is " + pformat(map(str, DEFAULT_TARGETS)), "<top>", 1)
trace("COMMAND_LINE_TARGETS is " + pformat(map(str, COMMAND_LINE_TARGETS)), "<top>", 1)
