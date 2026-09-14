/* words.c - dictionaries shared by the hangman, wordle, anagram and typing
 * families. Themes are used for letter-guessing and unscrambling; the
 * by-length lists back Wordle-style guessing at 4 to 8 letters.
 */
#include "engine.h"
#include "words.h"

const char *THEME_NAME[THEME_COUNT] = {
    "Animals", "Countries", "Capitals", "Computing", "Space", "Food",
    "Sports", "Films", "Music", "Nature", "Science", "History",
    "Occupations", "Colours", "Vehicles", "Instruments", "Weather",
    "Body Parts", "Tools", "Plants", "Mythology"
};

static const char *T_ANIMALS[] = {
"elephant","giraffe","kangaroo","dolphin","penguin","leopard","squirrel","hedgehog",
"crocodile","butterfly","rhinoceros","chimpanzee","albatross","porcupine","flamingo",
"octopus","salamander","wolverine","armadillo","chameleon"};
static const char *T_COUNTRIES[] = {
"australia","argentina","switzerland","netherlands","madagascar","kazakhstan",
"philippines","indonesia","portugal","denmark","morocco","ethiopia","colombia",
"thailand","vietnam","belgium","norway","iceland","uruguay","tanzania"};
static const char *T_CAPITALS[] = {
"canberra","brasilia","helsinki","reykjavik","bratislava","ljubljana","copenhagen",
"stockholm","budapest","bucharest","kathmandu","islamabad","nairobi","santiago",
"ottawa","vienna","lisbon","warsaw","ankara","dublin"};
static const char *T_COMPUTING[] = {
"algorithm","compiler","database","encryption","firewall","kernel","pointer",
"recursion","variable","function","protocol","register","semaphore","syntax",
"debugger","interface","hardware","bandwidth","repository","framework"};
static const char *T_SPACE[] = {
"asteroid","galaxy","nebula","supernova","telescope","satellite","meteorite",
"gravity","orbit","comet","eclipse","universe","astronaut","spacecraft","quasar",
"pulsar","cosmos","planet","solstice","celestial"};
static const char *T_FOOD[] = {
"spaghetti","chocolate","pineapple","sandwich","broccoli","cinnamon","avocado",
"baguette","croissant","dumpling","lasagne","meringue","omelette","pancake",
"risotto","tortilla","vanilla","waffle","yoghurt","zucchini"};
static const char *T_SPORTS[] = {
"badminton","basketball","cricket","cycling","football","gymnastics","handball",
"hockey","marathon","rowing","rugby","sailing","skiing","snowboard","swimming",
"tennis","triathlon","volleyball","wrestling","archery"};
static const char *T_FILMS[] = {
"casablanca","gladiator","inception","jaws","metropolis","nosferatu","psycho",
"rashomon","vertigo","alien","fargo","goodfellas","chinatown","amadeus",
"braveheart","titanic","avatar","interstellar","parasite","whiplash"};
static const char *T_MUSIC[] = {
"symphony","concerto","harmony","melody","rhythm","crescendo","staccato",
"arpeggio","octave","tempo","chorus","sonata","overture","baritone","soprano",
"percussion","orchestra","cadence","tremolo","nocturne"};
static const char *T_NATURE[] = {
"waterfall","glacier","rainforest","canyon","estuary","meadow","peninsula",
"volcano","savannah","tundra","wetland","coastline","plateau","lagoon","prairie",
"woodland","marshland","reef","delta","ravine"};
static const char *T_SCIENCE[] = {
"molecule","electron","catalyst","enzyme","isotope","photon","neutron","polymer",
"velocity","entropy","gravity","magnetism","osmosis","quantum","spectrum",
"thermal","viscosity","chromosome","antibody","radiation"};
static const char *T_HISTORY[] = {
"renaissance","revolution","monarchy","dynasty","colonial","medieval","crusade",
"republic","emperor","parliament","treaty","armistice","conquest","feudal",
"reformation","enlightenment","antiquity","prehistoric","napoleonic","abolition"};
static const char *T_OCCUPATIONS[] = {
"carpenter","architect","electrician","journalist","librarian","mechanic",
"pharmacist","plumber","surveyor","veterinary","accountant","biologist",
"chemist","dentist","engineer","geologist","historian","locksmith","navigator","teacher"};
static const char *T_COLOURS[] = {
"crimson","turquoise","magenta","lavender","emerald","scarlet","indigo","amber",
"cobalt","maroon","olive","saffron","teal","violet","beige","charcoal",
"burgundy","mustard","sapphire","vermilion"};
static const char *T_VEHICLES[] = {
"bicycle","motorcycle","helicopter","submarine","tractor","ambulance","bulldozer",
"catamaran","forklift","hovercraft","limousine","monorail","scooter","trawler",
"airship","caravan","freighter","gondola","rickshaw","snowplough"};
static const char *T_INSTRUMENTS[] = {
"accordion","bassoon","cello","clarinet","harpsichord","mandolin","oboe","piccolo",
"saxophone","trombone","trumpet","ukulele","violin","xylophone","harmonica",
"marimba","sitar","timpani","glockenspiel","euphonium"};
static const char *T_WEATHER[] = {
"blizzard","cyclone","drizzle","hailstone","humidity","hurricane","lightning",
"monsoon","overcast","precipitation","sleet","thunder","tornado","typhoon",
"barometer","frost","gale","mist","squall","tempest"};
static const char *T_BODY[] = {
"shoulder","abdomen","clavicle","diaphragm","elbow","femur","humerus","kidney",
"ligament","muscle","nostril","pancreas","ribcage","sternum","tendon","thumb",
"trachea","vertebra","wrist","ankle"};
static const char *T_TOOLS[] = {
"hammer","screwdriver","wrench","chisel","pliers","sandpaper","crowbar","mallet",
"spanner","drill","hacksaw","trowel","clamp","soldering","calipers","level",
"ratchet","grinder","scraper","stapler"};
static const char *T_PLANTS[] = {
"sunflower","dandelion","fern","bamboo","cactus","orchid","lavender","juniper",
"magnolia","sycamore","willow","clover","ivy","moss","thistle","nettle",
"hyacinth","primrose","sequoia","eucalyptus"};
static const char *T_MYTHOLOGY[] = {
"minotaur","centaur","phoenix","griffin","kraken","valkyrie","chimera","cyclops",
"pegasus","hydra","siren","titan","oracle","labyrinth","odyssey","olympus",
"ragnarok","sphinx","banshee","leviathan"};

const char **THEME_WORDS[THEME_COUNT] = {
    T_ANIMALS, T_COUNTRIES, T_CAPITALS, T_COMPUTING, T_SPACE, T_FOOD,
    T_SPORTS, T_FILMS, T_MUSIC, T_NATURE, T_SCIENCE, T_HISTORY,
    T_OCCUPATIONS, T_COLOURS, T_VEHICLES, T_INSTRUMENTS, T_WEATHER,
    T_BODY, T_TOOLS, T_PLANTS, T_MYTHOLOGY
};
const int THEME_SIZE[THEME_COUNT] = {
    20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20
};

/* ------------------------------------------------- guessable word lists */

static const char *W4[] = {
"able","acid","aged","also","area","army","away","baby","back","ball","band","bank",
"base","bath","bear","beat","been","beer","bell","belt","bend","best","bike","bird",
"bite","blue","boat","body","bomb","bond","bone","book","boom","boot","born","boss",
"both","bowl","bulk","burn","bush","busy","call","calm","came","camp","card","care",
"case","cash","cast","cell","chat","chip","city","club","coal","coat","code","cold",
"come","cook","cool","cope","copy","core","cost","crew","crop","dark","data","date",
"dawn","days","dead","deal","dean","dear","debt","deep","deny","desk","dial","diet",
"disc","disk","does","done","door","dose","down","draw","drew","drop","drug","dual",
"duke","dust","duty","each","earn","ease","east","easy","edge","else","even","ever",
"exit","face","fact","fail","fair","fall","farm","fast","fate","fear","feed","feel"};
static const char *W5[] = {
"about","above","actor","acute","admit","adopt","adult","after","again","agent",
"agree","ahead","alarm","album","alert","alike","alive","allow","alone","along",
"alter","among","anger","angle","angry","apart","apple","apply","arena","argue",
"arise","armed","aside","asset","audio","audit","avoid","awake","award","aware",
"beach","began","begin","being","below","bench","birth","black","blame","blind",
"block","blood","board","boost","bound","brain","brand","bread","break","breed",
"brief","bring","broad","broke","brown","build","built","buyer","cable","carry",
"catch","cause","chain","chair","chart","chase","cheap","check","chest","chief",
"child","china","chose","civil","claim","class","clean","clear","click","clock",
"close","coach","coast","could","count","court","cover","craft","crash","cream",
"crime","cross","crowd","crown","curve","cycle","daily","dance","dated","death",
"debut","delay","depth","doing","doubt","dozen","draft","drama","drawn","dream"};
static const char *W6[] = {
"accept","access","across","acting","action","active","actual","advice","advise",
"affect","afford","afraid","agency","agenda","almost","always","amount","animal",
"annual","answer","anyone","anyway","appeal","appear","around","arrive","artist",
"aspect","assess","assign","assist","assume","attack","attend","august","author",
"autumn","avenue","backed","backup","ballet","banner","barely","barrel","basket",
"battle","beauty","became","become","before","behalf","behind","belief","belong",
"berlin","better","beyond","bishop","border","bottle","bottom","bought","branch",
"brazil","breach","bridge","bright","broken","budget","burden","bureau","button",
"camera","cancer","cannot","canvas","carbon","career","castle","casual","caught",
"center","centre","chance","change","charge","choice","choose","chosen","church",
"circle","client","closed","closer","coffee","column","combat","coming","common"};
static const char *W7[] = {
"ability","absence","academy","account","accused","achieve","acquire","address",
"advance","adverse","advised","adviser","against","airline","airport","alcohol",
"alleged","already","analyst","ancient","another","anxiety","anybody","applied",
"appoint","arrange","arrival","article","assault","attempt","attract","auction",
"average","awkward","balance","banking","barrier","battery","bearing","beating",
"because","bedroom","believe","beneath","benefit","besides","between","bicycle",
"biggest","binding","brother","brought","builder","cabinet","caliber","calorie",
"caravan","capable","capital","captain","capture","careful","carrier","cushion",
"ceiling","central","century","certain","chamber","charter","channel","chapter",
"charity","chicken","circuit","citizen","classic","climate","closely","clothes"};
static const char *W8[] = {
"absolute","abstract","academic","accepted","accident","accuracy","accurate",
"achieved","acquired","activity","actually","addition","adequate","adjacent",
"adjusted","advanced","advisory","advocate","affected","aircraft","alliance",
"although","aluminum","analysis","announce","anything","anywhere","apparent",
"appeared","approach","approval","argument","arranged","arrested","artistic",
"assembly","assigned","assuming","attached","attacked","attitude","attorney",
"audience","aviation","bachelor","balanced","becoming","bringing","building",
"bulletin","business","calendar","campaign","capacity","carriage","casualty",
"category","catholic","cautious","ceremony","chairman","champion","chemical",
"cheerful","children","circular","civilian","clearing","clinical","clothing"};

const char **WORDS_BY_LEN[MAXLEN + 1] = { 0, 0, 0, 0, W4, W5, W6, W7, W8 };
const int WORDS_BY_LEN_COUNT[MAXLEN + 1] = {
    0, 0, 0, 0,
    (int)(sizeof(W4) / sizeof(W4[0])),
    (int)(sizeof(W5) / sizeof(W5[0])),
    (int)(sizeof(W6) / sizeof(W6[0])),
    (int)(sizeof(W7) / sizeof(W7[0])),
    (int)(sizeof(W8) / sizeof(W8[0]))
};

/* ------------------------------------------------------ typing passages */

const char *TYPING_MODE_NAME[TYPING_MODES] = {
    "Common Words", "Quotations", "Code Snippets", "Numbers", "Punctuation",
    "Long Passage", "Sixty Second", "Accuracy Mode", "Pangrams"
};

static const char *TY_COMMON[] = {
"the time has come to make a change and take the first step forward today",
"work with people you trust and the hard parts become much easier to face",
"a small amount of steady effort will beat a large amount of good intention"};
static const char *TY_QUOTES[] = {
"the best way to predict the future is to invent it one careful step at a time",
"simplicity is prerequisite for reliability and clarity beats cleverness",
"programming is the art of telling another human what one wants the computer to do"};
static const char *TY_CODE[] = {
"for (int i = 0; i < n; i++) { total += values[i]; }",
"if (ptr == NULL) { return -1; } else { free(ptr); ptr = NULL; }",
"while (head != NULL) { struct node *next = head->next; head = next; }"};
static const char *TY_NUMBERS[] = {
"1024 4096 16384 65536 262144 1048576 4194304 16777216",
"3141592653 2718281828 1618033988 1414213562 1732050807",
"42 1970 2038 8080 3306 5432 27017 65535 131072 524288"};
static const char *TY_PUNCT[] = {
"well, that's odd -- isn't it? (maybe not!) she said; then left.",
"\"stop,\" he said. \"don't move!\" -- but it was already too late...",
"items: one; two; three. total? six. cost: $4.50 (plus tax)."};
static const char *TY_LONG[] = {
"the quick brown fox jumps over the lazy dog while the clock strikes noon and "
"the rain begins to fall against the window of the quiet library where nobody "
"has spoken for hours and the only sound is the turning of a single page"};
static const char *TY_SIXTY[] = {
"type steadily for a full minute and let the rhythm settle rather than rushing "
"each individual word because accuracy compounds faster than raw speed does"};
static const char *TY_ACCURACY[] = {
"precision matters more than pace when every single character must be correct",
"slow down and place each keystroke deliberately until the errors fall away"};
static const char *TY_PANGRAM[] = {
"the quick brown fox jumps over the lazy dog",
"pack my box with five dozen liquor jugs",
"how vexingly quick daft zebras jump"};

const char **TYPING_TEXT[TYPING_MODES] = {
    TY_COMMON, TY_QUOTES, TY_CODE, TY_NUMBERS, TY_PUNCT,
    TY_LONG, TY_SIXTY, TY_ACCURACY, TY_PANGRAM
};
const int TYPING_TEXT_COUNT[TYPING_MODES] = { 3, 3, 3, 3, 3, 1, 1, 2, 3 };

/* ---------------------------------------------------------------- helpers */

int theme_index(const char *name)
{
    int i;
    if (!name || !*name) return -1;
    for (i = 0; i < THEME_COUNT; i++)
        if (strcmp(THEME_NAME[i], name) == 0) return i;
    return -1;
}

const char *theme_pick(int theme)
{
    if (theme < 0 || theme >= THEME_COUNT) theme = 0;
    return THEME_WORDS[theme][rnd(THEME_SIZE[theme])];
}

int word_valid(const char *w, int len)
{
    int i;
    if (len < MINLEN || len > MAXLEN) return 0;
    for (i = 0; i < WORDS_BY_LEN_COUNT[len]; i++)
        if (strcmp(WORDS_BY_LEN[len][i], w) == 0) return 1;
    return 0;
}
