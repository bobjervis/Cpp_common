#include "../common/platform.h"
#include "../common/machine.h"
#include "../common/file_system.h"
#include "../common/locale.h"
#include "../common/common_test.h"
#include "../engine/engine.h"
#include "../engine/game.h"
#include "../engine/game_map.h"
#include "../engine/global.h"
#include "../dance/dance.h"
#include "../dance/dance_ui.h"
#include "../test/test.h"
#include "../ui/ui.h"

namespace alys2 {
void launch(int argc, char** argv);
}

static void dumpBinaryFile(const string& filename);
static void extractNames(const string& namesFile, const string& xmpFile, double lat1, double lat2, double long1, double long2, const string& xmlOutFile);
static void filterNames(const string& outputFilename, int inputCount, char** inputs);
static void help();

int main(int argc, char** argv) {
	platform::setup();
	display::initialize(argv[0]);
	engine::setupLog();
	global::binFolder = fileSystem::absolutePath(fileSystem::directory(argv[0]));

		// Look in the normal build location so that Europa
		// product developers can edit the original sources
		// for scenarios, etc. while end-users edit the
		// built copy.

	global::dataFolder = global::binFolder + "/..";
	global::userFolder = fileSystem::absolutePath(ui::findUserFolder());
	if (!fileSystem::exists(global::dataFolder + "/images/combat.bmp")){
		global::dataFolder = global::binFolder + "/../data";
		if (!fileSystem::exists(global::dataFolder + "/images/combat.bmp"))
			fatalMessage("Cannot find working data folder location\n"
						 "Looking for images/combat.bmp");
	}
	global::namesFile = global::dataFolder + "/reference/wwii.xml";
	global::terrainKeyFile = global::dataFolder + "/reference/terrainKey.xml";
	global::theaterFilename = global::dataFolder + "/reference/wwii.europe.theater";
	global::parcMapsFilename = global::dataFolder + "/reference/parcMaps.xml";
	bool testRun = false;
	const char* command = argv[0];
	while (argc > 1 && argv[1][0] == '-' && argv[1][1] == '-') {
		string arg(argv[1]);
		if (locale::startsWithIgnoreCase(arg, "--importance=")) {
			global::kmPerHex = atof(arg.substr(13).c_str());
		} else if (locale::startsWithIgnoreCase(arg, "--names=")) {
			global::namesFile = arg.substr(8);
		} else if (locale::startsWithIgnoreCase(arg, "--location=")) {
			global::initialPlace = arg.substr(11);
		} else if (locale::startsWithIgnoreCase(arg, "--ai=")) {
			global::aiForces = arg.substr(5);
		} else if (locale::startsWithIgnoreCase(arg, "--rotation=")) {
			global::rotation = arg.substr(11);
		} else if (locale::startsWithIgnoreCase(arg, "--theater=")) {
			global::theaterFilename = arg.substr(10);
		} else if (locale::startsWithIgnoreCase(arg, "--seed=")) {
			printf("Random seed to %d\n", arg.substr(7).toInt());
			global::randomSeed = arg.substr(7).toInt();
		} else if (arg == "--test") {
			testRun = true;
		} else if (arg == "--toe") {
			global::oobSortOrder = engine::OS_TOE;
		} else if (arg == "--play") {
			global::playOneTurnOnly = true;
		} else if (arg == "--log") {
			engine::logToggle.set_value(true);
		} else if (arg == "--verbose") {
			dance::verboseOutput = true;
		} else if (arg == "--verboseBreathe") {
			dance::verboseBreathing = true;
		} else if (arg == "--verboseParse") {
			dance::verboseParsing = true;
		} else if (arg == "--verboseMatch") {
			dance::verboseMatching = true;
		} else if (arg == "--no_ui") {
			dance::showUI = false;
		} else if (arg == "--ignore_state") {
			dance::loadState = false;
		} else if (arg == "--boevoi") {
			global::oobSortOrder = engine::OS_BOEVOI;
		} else
			help();
		argc--;
		argv++;
	}
	global::reportError = ui::reportError;
	global::reportInfo = ui::reportInfo;
 	if (argc == 1)
		help();
	if (strcmp(argv[1], "alys2") == 0)
		alys2::launch(argc - 1, argv + 1);
	else if (strcmp(argv[1], "game") == 0)
		ui::launchGame();
	else if (strcmp(argv[1], "newmap") == 0) {
		if (argc != 5)
			help();
		ui::launchNewMap(argv[2], atoi(argv[3]), atoi(argv[4]));
	}
//	else if (strcmp(argv[1], "laeamap") == 0)
//		ui::launchNewLaeaMap(argv[2]);
	else if (strcmp(argv[1], "dance") == 0)
		dance::launch(argc - 2, argv + 2);
	else if (strcmp(argv[1], "edit") == 0)
		ui::launch(argc - 2, argv + 2);
	else if (strcmp(argv[1], "test") == 0) {
		if (argc == 2)
			help();
		script::setCommandPrefix(fileSystem::absolutePath(command) + " test");
		engine::initTestObjects();
		dance::initTestObjects();
		ui::initTestObjects();
		initCommonTestObjects();
		exit(test::launch(argc - 2, argv + 2));
	}
/*
	else if (process.arguments[0] == "sortoob")
		sortOOB(process.arguments[1], process.arguments[2], process.arguments[3])
 */
	else if (strcmp(argv[1], "extract") == 0) {
		if (argc != 9)
			help();
		extractNames(argv[2], argv[3], atof(argv[4]), atof(argv[5]), atof(argv[6]), atof(argv[7]), argv[8]);
	} else if (strcmp(argv[1], "names") == 0)
		filterNames(argv[2], argc - 3, argv + 3);
	else if (strcmp(argv[1], "dump") == 0)
		dumpBinaryFile(argv[2]);
/*
	else if (process.arguments[0] == "stats")
		statNames()
	else if (process.arguments[0] == "weapons")
		weaponsTable()
 */
	else if (argc == 2) {
		if (!ui::launchGame(argv[1])) {
			printf("Could not launch game from '%s'\n", argv[1]);
			engine::printErrorMessages();
			printf("\n");
			help();
		}
	} else
	if (testRun)
		return test::run();
	else
		return display::loop();
}

double getDegrees(const char*s, int len) {
	int deg = 0;
	int min = 0;
	int sec = 0;
	int neg = false;
	if (char(s[0]) == '-') {
		s++;
		len--;
		neg = true;
	}
	if (len >= 5) {
		int mult = 1;
		for (int i = len - 5; i >= 0; i--){
			deg += (char(s[i]) - '0') * mult;
			mult = mult * 10;
		}
	}
	if (len >= 4)
		min = (char(s[len - 4]) - '0') * 10 + (char(s[len - 3]) - '0');
	else if (len == 3)
		min = char(s[len - 3]) - '0';
	if (len >= 2)
		sec = (char(s[len - 2]) - '0') * 10 + (char(s[len - 1]) - '0');
	else if (len == 1)
		sec = char(s[len - 1]) - '0';
	double res = float(deg) + float(min) / 60 + float(sec) / 3600;
	if (neg)
		res = -res;
	return res;
}

static void printDegrees(FILE* out, char* deg, int len) {
	if (deg[0] == '-') {
		fprintf(out, "-");
		deg++;
		len--;
	}
	switch (len) {
	case	0:
		fprintf(out, "0d0");
		break;
	case	1:
		fprintf(out, "0d0m%1.1s", deg);
		break;
	case	2:
		fprintf(out, "0d0m%2.2s", deg);
		break;
	case	3:
		fprintf(out, "0d%1.1sm%2.2s", deg, deg + 1);
		break;
	case	4:
		fprintf(out, "0d%2.2sm%2.2s", deg, deg + 2);
		break;
	case	5:
		fprintf(out, "%1.1sd%2.2sm%2.2s", deg, deg + 1, deg + 3);
		break;
	default:
		fprintf(out, "%2.2sd%2.2sm%2.2s", deg, deg + 2, deg + 4);
	}
}

static void extractNames(const string& namesFile, const string& xmpFile, double lat1, double lat2, double long1, double long2, const string& xmlOutFile) {
	engine::HexMap* map = engine::loadHexMap(xmpFile, global::namesFile, global::terrainKeyFile);
	if (map == null)
		fatalMessage("Cannot find map description " + xmpFile);
	FILE* out = fileSystem::createTextFile(xmlOutFile);
	FILE* in = fileSystem::openTextFile(namesFile);
	if (in == null)
		fatalMessage("Cannot find input names file " + namesFile);
//	m := new text.MemoryStream()
	if (lat1 > lat2){
		double x = lat2;
		lat2 = lat1;
		lat1 = x;
	}

		// now lat1 <= lat2

	if (long1 > long2){
		double x = long2;
		long2 = long1;
		long1 = x;
	}

		// now long1 <= long2


		// Copy the data

	char* xlat;
	char* xlong;
	int xlatLen;
	int xlongLen;

	bool readAnything = false;

	int b;

	fprintf(out, "<root>\n");
	for (;;){
		char buffer[1024];

		char* cp = fgets(buffer, sizeof buffer, in);
		if (cp == null)
			break;
		double lat0;
		double long0;
		string dsg, country, native, name;
		char* found = strchr(cp, '\t');
		if (found) {
			xlat = cp;
			xlatLen = found - cp;
			lat0 = getDegrees(cp, found - cp);
			cp = found + 1;
		} else
			continue;
		found = strchr(cp, '\t');
		if (found) {
			xlong = cp;
			xlongLen = found - cp;
			long0 = getDegrees(cp, found - cp);
			cp = found + 1;
		} else
			continue;
		found = strchr(cp, '\t');
		if (found) {
			dsg = string(cp, found - cp);
			cp = found + 1;
		} else
			continue;
		found = strchr(cp, '\t');
		if (found) {
			country = string(cp, found - cp);
			cp = found + 1;
		} else
			continue;
		found = strchr(cp, '\t');
		if (found) {
			native = string(cp, found - cp);
			cp = found + 1;
		} else
			continue;
		found = strchr(cp, '\n');
		if (found) {
			name = string(cp, found - cp);
			cp = found + 1;
		} else
			continue;
		if (lat1 > lat0 ||
			lat0 > lat2 ||
			long1 > long0 ||
			long0 > long2)
			continue;
		engine::xpoint p = map->latLongToHex(lat0, long0);
		if (!map->valid(p))
			continue;
		fprintf(out, "%3.3d/%3.3d\t<place name=\"%s\" lat=", p.y, p.x, name.c_str());
		printDegrees(out, xlat, xlatLen);
		fprintf(out, " long=");
		printDegrees(out, xlong, xlongLen);
		fprintf(out, " imp=0 country=%s native=%s dsg=%s/>\n", country.c_str(), native.c_str(), dsg.c_str());
	}
	fclose(in);
	fprintf(out, "</root>\n");
	fclose(out);
	exit(0);
}

static void help() {
	printf(
		"Use is: europa [ <options> ... ] <subcommand>\n"
		"  subcommands are:\n"
		"     edit [ <file> ... ]\n"
		"     test <script> ...\n"
		"     newmap <map file name> <rows> <columns>\n"
//		"     parcmap <parc map file designation>\n"
//		"     sortoob <scenario file> <combatant> <output file>\n"
		"     extract <.names files> <.xmp file> <lat1> <lat2> <long1>\n"
		"               <long2> <.xml output file>\n"
		"     names <.names file> <source NIMA data file(s)>\n"
		"     dump <.hsv file>\n"
//		"     stats <output file> <.names file(s)>\n"
//		"     xml <.xml output file> <.xmp file> <.names file(s)>\n"
//		"     <game save (.hsv file)>\n"
		"     <scenario file (.scn file)>\n"
		"  options are:\n"
		"     --importance=N     Set the importance scale for edit sub-command\n"
		"     --names=<file>     Set the names file to use.\n"
		"     --location=<place> Center map on given place.\n"
		"     --ai=<force>,...   Identify computer player(s).\n"
		"     --toe              Sort OOB by toe values.\n"
		"     --boevoi           Sort OOB by boevoi sostav categories.\n"
		"An empty command line starts a new game.\n"
		);
	exit(1);
}

static void filterNames(const string& outputFilename, int inputCount, char** inputs) {
	FILE* out = fileSystem::createTextFile(outputFilename);

	for (int i = 0; i < inputCount; i++) {
		fileSystem::Directory d(fileSystem::directory(inputs[i]));
		d.pattern(fileSystem::basename(inputs[i]));
		if (!d.first())
			continue;
		do {
			string filename = d.currentName();
//			fprintf(out, "%s\n", filename.c_str());
			FILE* in = fileSystem::openTextFile(filename);

			string m;

				// Read the schema

			struct Column {
				Column() {
					next = null;
					copy = false;
				};

				Column*		next;
				string		value;
				boolean		copy;
			};


			Column* schema = null;
			Column* lastSchema;
			for (;;) {
				int b = fgetc(in);
				if (b == EOF)
					break;
				if (b == '\t' ||
					b == '\n') {
					Column* c = new Column;
					c->next = null;
					c->value = m;
					c->copy = (m == "DMS_LAT" ||
							   m == "DMS_LONG" ||
							   m == "FULL_NAME_ND" ||
							   m == "DSG" ||
							   m == "CC1" ||
							   m == "NT");
					if (schema == null)
						schema = c;
					else
						lastSchema->next = c;
					lastSchema = c;
//					fprintf(out, "%s(%d)\t", m.c_str(), c->copy);
					m.clear();
					if (b == '\n')
						break;
				} else
					m.push_back(b);
			}
//			fprintf(out, "\n");

				// Copy the data

			string dmsLat;
			string dmsLong;
			char buffer[512];
			int b;

			do {
				bool shouldEcho = false;
				bool suppressEcho = false;

				b = EOF;
				for (Column* s = schema; s != null; s = s->next) {
					for (;;) {
						b = fgetc(in);
						if (b == EOF)
							break;
						if (b == '\t' ||
							b == '\n') {
							if (s->copy) {
								if (s->value == "DMS_LAT") {
									dmsLat = m;
									int lat = (dmsLat[0] - '0') * 10 + (dmsLat[1] - '0');
									if (lat < 38 || lat > 72)
										suppressEcho = true;
								} else if (s->value == "DMS_LONG") {
									dmsLong = m;
									bool doThis = dmsLong.size() <= 6;
									int xlong;
									if (dmsLong[0] == '-')
										xlong = 0;
									else if (dmsLong.size() == 6)
										xlong = (dmsLong[0] - '0') * 10 + (dmsLong[1] - '0');
									else if (dmsLong.size() == 5)
										xlong = dmsLong[0] - '0';
									else
										xlong = 0;
									if (xlong > 60)
										suppressEcho = true;
								} else {
									if (!suppressEcho && s->value == "DSG") {
										if (m.beginsWith("ADM")) {
											m.clear();
											break;
										}
										shouldEcho = true;
										fprintf(out, "%s\t%s", dmsLat.c_str(), dmsLong.c_str());
									}
								}
								if (shouldEcho)
									fprintf(out, "\t%s", m.c_str());
							}
							m.clear();
							break;
						} else
							m.push_back(b);
					}
					if (b != '\t')
						break;
				}
				if (shouldEcho)
					fputc('\n', out);
			} while (b != EOF);
			fclose(in);
		} while (d.next());
	}
	fclose(out);
	exit(0);
}

static void dumpBinaryFile(const string& filename) {
	engine::initForGame();
	fileSystem::Storage s(filename, &global::storageMap);

	if (s.dump(global::dataFolder + "/test/scripts/hsv_schema.ets"))
		exit(0);
	else
		exit(1);
}
