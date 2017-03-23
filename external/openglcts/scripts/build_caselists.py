# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------
# drawElements Quality Program utilities
# --------------------------------------
#
# Copyright 2015 The Android Open Source Project
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
#-------------------------------------------------------------------------

import os
import sys
import string
import argparse
import tempfile
import shutil

sys.path.append(os.path.join(os.path.dirname(__file__), "..", "..", "..", "scripts"))

from build.common import *
from build.config import *
from build.build import *


class Module:
	def __init__ (self, name, api):
		self.name		= name
		self.api        = api

MODULES = [
	Module("dEQP-EGL",		"EGL"),
	Module("dEQP-GLES2",	"GLES2"),
	Module("dEQP-GLES3",	"GLES3"),
	Module("dEQP-GLES31",	"GLES31"),
	Module("KHR-GLES3",		"GLES3"),
	Module("KHR-GLES2",		"GLES2"),
	Module("KHR-GLES31",	"GLES31"),
	Module("KHR-GLES32",	"GLES32"),
	Module("GTF-GLES2",		"GLES2" ),
	Module("GTF-GLES3",		"GLES3" ),
	Module("GTF-GLES31",	"GLES31"),
]
GLCTS_BIN_NAME = "glcts"
GLCTS_DIR_NAME = "external/openglcts/modules/"
DEFAULT_BUILD_DIR	= os.path.join(tempfile.gettempdir(), "deqp-caselists", "{targetName}-{buildType}")
DEFAULT_TARGET		= "null"

def getModuleByName (name):
	for module in MODULES:
		if module.name == name:
			return module
	else:
		raise Exception("Unknown module %s" % name)

def getBuildConfig (buildPathPtrn, targetName, buildType):
	buildPath = buildPathPtrn.format(
		targetName	= targetName,
		buildType	= buildType)

	return BuildConfig(buildPath, buildType, ["-DDEQP_TARGET=%s" % targetName])

def getModulesPath (buildCfg):
	return os.path.join(buildCfg.getBuildDir(), GLCTS_DIR_NAME)

def getCaseListFileName (module, caseListType):
	return "%s-cases.%s" % (module.name, caseListType)

def getCaseListPath (buildCfg, module, caseListType):
	workDir = getModulesPath(buildCfg)

	return os.path.join(workDir, getCaseListFileName(module, caseListType))

def genCaseList (buildCfg, generator, caseListType):
	workDir = getModulesPath(buildCfg)

	pushWorkingDir(workDir)

	try:
		binPath = generator.getBinaryPath(buildCfg.getBuildType(), os.path.join(".", GLCTS_BIN_NAME))
		execute([binPath, "--deqp-runmode=%s-caselist" % caseListType])
	finally:
		popWorkingDir()

def genAndCopyCaseList (buildCfg, generator, module, dstDir, caseListType):
	caseListFile	= getCaseListFileName(module, caseListType)
	srcPath			= getCaseListPath(buildCfg, module, caseListType)
	dstPath			= os.path.join(dstDir, caseListFile)

	if os.path.exists(srcPath):
		os.remove(srcPath)

	genCaseList(buildCfg, generator, module, caseListType)

	if not os.path.exists(srcPath):
		raise Exception("%s not generated" % srcPath)

	shutil.copyfile(srcPath, dstPath)
