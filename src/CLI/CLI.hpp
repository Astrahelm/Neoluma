#pragma once
#include "CLIHelperFunctions.hpp"
#include "HelperFunctions.hpp"

#include "Libraries/Asker/Asker.hpp"

#include <vector>
#include <map>
#include <chrono>

// CLIArgs is a struct that allows me to parse CLI arguments
struct CLIArgs {
    std::string command; // parses commands like new, build, run, check and help
    std::map<std::string, std::string> options; // parses --arguments with_values.nlp
    std::vector<std::string> positional; // anything that is not an --argument
};

// Argument parsing
CLIArgs parseArgs(int argc, char** argv);

// ==== Main functions ====

void build(const std::string& nlpFile); // Compiles Neoluma program into a binary executable
void run(const std::string& nlpFile); // Runs the code interpreted way. Useful for testing.
void check(const std::string& nlpFile, bool jsonOutput = false); // Checks code on errors. Doesn't generate any binaries
void createProject(ProjectConfig config); // Creates a project
void createProject(); // Creates a project (Without ProjectConfig)

// Help function that just tells details about compiler and it's CLI.
void printHelp();

// Licenses templates
class Licenses {
    static inline const std::string MIT_TEMPLATE = {
        #embed "LicenseTemplates/MIT.txt"
    };
    static inline const std::string APACHE_TEMPLATE = {
        #embed "LicenseTemplates/Apache.txt"
    };
    static inline const std::string GNUGPLv3_TEMPLATE = {
        #embed "LicenseTemplates/GNUGPLv3.txt"
    };
    static inline const std::string BSDv2Simplified_TEMPLATE = {
        #embed "LicenseTemplates/BSDv2Simplified.txt"
    };
    static inline const std::string BSDv3NewRevised_TEMPLATE = {
        #embed "LicenseTemplates/BSDv3NewRevised.txt"
    };
    static inline const std::string BoostV1_TEMPLATE = {
        #embed "LicenseTemplates/BoostV1.txt"
    };
    static inline const std::string CC0v1_TEMPLATE = {
        #embed "LicenseTemplates/CC0v1.txt"
    };
    static inline const std::string EclipseV2_TEMPLATE = {
        #embed "LicenseTemplates/EclipseV2.txt"
    };
    static inline const std::string GNUAGPLv3_TEMPLATE = {
        #embed "LicenseTemplates/GNUAGPLv3.txt"
    };
    static inline const std::string GNUGPLv2_TEMPLATE = {
        #embed "LicenseTemplates/GNUGPLv2.txt"
    };
    static inline const std::string GNULGPLv2_1_TEMPLATE = {
        #embed "LicenseTemplates/GNULGPLv2_1.txt"
    };
    static inline const std::string MozillaV2_TEMPLATE = {
        #embed "LicenseTemplates/MozillaV2.txt"
    };
    static inline const std::string Unlicense_TEMPLATE = {
        #embed "LicenseTemplates/Unlicense.txt"
    };
public:
    // Available identifiers: mit, apache, gpl2, gpl3, bsd2, bsd3, boost, cc0, eclipse, agpl, lgpl, mozilla, unlicense. Otherwise returns specific message
    static std::string checkLicense(ProjectConfig config, License license) {
        if (license == License::MIT) return MIT(listAuthors(config.author));
        if (license == License::Apache) return Apachev2();
        if (license == License::GPL2) return GNUGPLv2();
        if (license == License::GPL3) return GNUGPLv3();
        if (license == License::BSD2) return BSDv2Simplified(listAuthors(config.author));
        if (license == License::BSD3) return BSDv3NewRevised(listAuthors(config.author));
        if (license == License::Boost) return Boostv1();
        if (license == License::CC0) return CC0v1();
        if (license == License::Eclipse) return Eclipsev2();
        if (license == License::AGPL) return GNUAGPLv3();
        if (license == License::LGPL) return GNULGPLv2_1();
        if (license == License::Mozilla) return Mozillav2();
        if (license == License::Unlicense) return Unlicense();
        return "We haven't found licenses for your case. Please delete this text and insert your license here, or delete the license file completely.";
    }

    /* A short and simple permissive license with conditions only requiring preservation of copyright and license notices. 
    Licensed works, modifications, and larger works may be distributed under different terms and without source code. */
    static std::string MIT(std::string author) {
        const auto year = formatStr("{:%Y}", std::chrono::system_clock::now());
        return formatStr(MIT_TEMPLATE, year, author);
    }

    /* A permissive license whose main conditions require preservation of copyright and license notices. 
    Contributors provide an express grant of patent rights. Licensed works, modifications, and larger works may be 
    distributed under different terms and without source code. */
    static std::string Apachev2() {
        return APACHE_TEMPLATE;
    }

    /*Permissions of this strong copyleft license are conditioned on making available complete source code of licensed works 
    and modifications, which include larger works using a licensed workunder the same license. Copyright and license notices 
    must be preserved. Contributors provide an express grant of patent rights.*/
    static std::string GNUGPLv3() {
        return GNUGPLv3_TEMPLATE;
    }

    /*A permissive license that comes in two variants, the BSD 2-Clause and BSD 3-Clause. 
    Both have very minute differences to the MIT license.*/
    static std::string BSDv2Simplified(std::string author){
        const auto year = formatStr("{:%Y}", std::chrono::system_clock::now());
        return formatStr(BSDv2Simplified_TEMPLATE, year, author);
    }

    /*A permissive license similar to the BSD 2-Clause License, but with a 3rd clause that prohibits others from using the name
    of the copyright holder or its contributors to promote derived products without written consent.*/
    static std::string BSDv3NewRevised(std::string author) {
        const auto year = formatStr("{:%Y}", std::chrono::system_clock::now());
        return formatStr(BSDv3NewRevised_TEMPLATE, year, author);
    }

    /*A simple permissive license only requiring preservation of copyright and license notices for source (and not binary) distribution. 
    Licensed works, modifications, and larger works may be distributed under different terms and without source code.*/
    static std::string Boostv1() {
        return BoostV1_TEMPLATE;
    }

    /*The Creative Commons CC0 Public Domain Dedication waives copyright interest in a work you've created and dedicates 
    it to the world-wide public domain. Use CC0 to opt out of copyright entirely and ensure your work has the widest reach. 
    As with the Unlicense and typical software licenses, CC0 disclaims warranties. CC0 is very similar to the Unlicense.*/
    static std::string CC0v1() {
        return CC0v1_TEMPLATE;
    }

    /*This commercially-friendly copyleft license provides the ability to commercially license binaries; a modern royalty-free patent 
    license grant; and the ability for linked works to use other licenses, including commercial ones.*/
    static std::string Eclipsev2() {
        return EclipseV2_TEMPLATE;
    }

    /*Permissions of this strongest copyleft license are conditioned on making available complete source code of licensed works 
    and modifications, which include larger works using a licensed work, under the same license. Copyright and license notices 
    must be preserved. Contributors provide an express grant of patent rights. When a modified version is used to provide 
    a service over a network, the complete source code of the modified version must be made available.*/
    static std::string GNUAGPLv3() {
        return GNUAGPLv3_TEMPLATE;
    }

    /* The GNU GPL is the most widely used free software license and has a strong copyleft requirement.
    When distributing derived works, the source code of the work must be made available under the same license.
    There are multiple variants of the GNU GPL, each with different requirements. */
    static std::string GNUGPLv2() {
        return GNUGPLv2_TEMPLATE;
    }

    /* Primarily used for software libraries, the GNU LGPL requires that derived works be licensed under the same license, but works that
    only link to it do not fall under this restriction. There are two commonly used versions of the GNU LGPL.*/
    static std::string GNULGPLv2_1() {
        return GNULGPLv2_1_TEMPLATE;
    }

    /*Permissions of this weak copyleft license are conditioned on making available source code of licensed files and modifications of those 
    files under the same license (or in certain cases, one of the GNU licenses). Copyright and license notices must be preserved. 
    Contributors provide an express grant of patent rights. However, a larger work using the licensed work may be distributed under 
    different terms and without source code for files added in the larger work.*/
    static std::string Mozillav2() {
        return MozillaV2_TEMPLATE;
    }

    /*A license with no conditions whatsoever which dedicates works to the public domain. Unlicensed works, modifications, 
    and larger works may be distributed under different terms and without source code.*/
    static std::string Unlicense() {
        return Unlicense_TEMPLATE;
    }
};