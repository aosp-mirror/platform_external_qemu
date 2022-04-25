// Copyright 2022 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include <string>
#include "android/cmdline-definitions.h"
#include "android/emulation/control/globals_agent.h"

AndroidOptions* sAndroid_cmdLineOptions;
std::string sCmdlLine;
LanguageSettings s_languageSettings = {0};
AUserConfig* s_userConfig = nullptr;

static const QAndroidGlobalVarsAgent globalVarsAgent = {

        .language = []() { return &s_languageSettings; },
        .userConfig = []() { return s_userConfig; },
        .android_cmdLineOptions = []() { return sAndroid_cmdLineOptions; },
        .inject_cmdLineOptions =
                [](AndroidOptions* opts) { sAndroid_cmdLineOptions = opts; },
        .has_cmdLineOptions =
                []() {
                    return globalVarsAgent.android_cmdLineOptions() != nullptr;
                },
        .android_cmdLine = []() { return (const char*)sCmdlLine.c_str(); },
        .inject_android_cmdLine =
                [](const char* cmdline) { sCmdlLine = cmdline; },
        .inject_language =
                [](char* language, char* country, char* locale) {
                    s_languageSettings.language = language;
                    s_languageSettings.country = country;
                    s_languageSettings.locale = locale;
                    s_languageSettings.changing_language_country_locale =
                            language || country || locale;
                },
        .inject_userConfig =
                [](AUserConfig* config) { s_userConfig = config; }};

extern "C" const QAndroidGlobalVarsAgent* const gQAndroidGlobalVarsAgent =
        &globalVarsAgent;
