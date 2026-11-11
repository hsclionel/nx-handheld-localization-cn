// pinyin: pinyin-initial lookup for CJK Unified Ideographs, for search and sorting.
//
// A name such as 吞食天地 folds to the key "tstd" (each ideograph becomes the first letter of its most
// common reading), so it can be found by typing "tstd" and filed under T in the letter-jump list.
//
// The table (U+4E00..U+9FFF, one letter per code point, '.' = no reading) is the one from
// yjvaselines/nx-redux-cn, built from the pypinyin data set (MIT). Polyphones use their most common
// reading, so 长 is "c" even in 长生不老.
//
// Single-header like i18n.h: declarations here, implementation in the one translation unit that
// defines PINYIN_IMPL before including this file (entry_names.c).

#ifndef PINYIN_H
#define PINYIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Decode the UTF-8 sequence at s into *cp. Returns its length in bytes (1 for ASCII, 1 for a bad byte).
int Pinyin_decode(const char* s, uint32_t* cp);

// First letter 'a'..'z' of the reading of code point cp, or 0 when it has none (not an ideograph in range, or no reading).
char Pinyin_initial(uint32_t cp);

// Fold a UTF-8 string into a search key: ideographs become their initial, ASCII letters are lowercased, digits are
// kept, everything else is dropped. True when at least one character landed in `out`.
bool Pinyin_initials(const char* utf8, char* out, size_t out_size);

// Fold a UTF-8 string into a sort key: ideographs with a reading become their initial; every other character is kept
// (ASCII lowercased), so names sort as Latin text and Chinese names file under their initials. Returns the length.
size_t Pinyin_sortKey(const char* utf8, char* out, size_t out_size);

// True when the string holds a byte >= 0x80.
bool Pinyin_hasMultibyte(const char* s);

#endif // PINYIN_H

#ifdef PINYIN_IMPL
#ifndef PINYIN_IMPL_DONE
#define PINYIN_IMPL_DONE

#define PINYIN_START 0x4E00u
#define PINYIN_COUNT 20992u

static const char PINYIN_DATA[] =
	"ydkqsxhwzssxjbymgcczqpssqbycdscdqldylybs"
	"gjgyqzjjfgcclzzbwdwzjljpfyynwjjtmyyzwzhf"
	"lyppqhgccyyymjqyxxgjxhsdsjnjjsmhmlzrxyfs"
	"ngsyczqzggllyjlmyzssecykyyhqwjssggyxyqyj"
	"twkdjhychmyxjtlxjyqbyxdldwrrjjwysrldzjpc"
	"bzjjbrcfslbczstzfxxthtrqggbdlyccscymmrfc"
	"yqzpwwjjyfcrwfdfzqpyddwyxkyjawjffxjpdftz"
	"yhhycyswccyqsclcxxwzzxnbgnnxbxlzsqcbsgpy"
	"syzdhmdzbqbzcwdzzyytzhbtsyyfzgntnxqywqsk"
	"bphhlxgybfmjebjhhgqtjcysxstkzglyckglysmz"
	"xyalmeldccxgzyrcxsdltjzcqkcnnjwhjczzcqlj"
	"ststbnxbtyxceqxgkwjyflzqlyhjqspsfxlfpbyq"
	"xxxydcczylllsjxfhjxpjbcffyabyxbhczbjyclw"
	"lczggbtssmdtjcxpthyqtgjjscjfzkjzjqnlzwls"
	"lhdzbwjncjzyzsqnycqyrzcjjwybrtwpyftwexcs"
	"kdzctbxhyzcyyjxzcfbzzmjyxxcdczottbzljwfc"
	"gszsxfyrlnyjmbdthjxsqjccsbxyytsyfbjdztgb"
	"cnclcyzzbsacyzzscjcshzqydxlbpjllmqxtydzx"
	"sqjtzpxlcglqccwjbhctdjjsfxjejjtlbgxsxjmy"
	"jjqpfzasyjncydjxkjcdjszcbartcclnjqmwnqnc"
	"lllkbybzzsyhqcltwlccrshllzntylnewyzyxczx"
	"xgdkdmtcedejtsyys.dqdfmsd.jlhrwnqlybglxh"
	"lgtgxbqjdzfyjsjyjcjmrnymgrcjczgjmzmgxmmr"
	"yxkjnymsgmzjymklfxmbdtgfbhcjhkylpfmdxlqj"
	"jsmtqgzsjlqdldgjycalcmzcsdjllnxdjffffjcz"
	"fmzffpfkhkgdpqxktacjdhhzdddrrcfqyjkqccwj"
	"dxhwjlyllzgcfcqdsmlzpbjjplsbcjggdckkdezs"
	"qsckjgcgkdjtjllzycxklqscgjcltfpcqczgwbjd"
	"qsdjjbyjhsjddwgfsjgdkccctllpspkjgqjhzzlj"
	"plgjgjjthjjyjzcjmlzlyqbgjwmljkxzdznjqsyz"
	"mljlljkywxmkjlhskjgbmclyymkxjqlbmclkmdxx"
	"kwyxwslmlpsjqjcqxyjfjtjdxmxxllcrqbsyjbgw"
	"ywxggbcyxpjtgpepfgdjgbhbnsfjyzjkjkhxqfgq"
	"zkfhygkhdgllsdjjxpqykybnqsxqnszswhbsxwhx"
	"wbzzxdmndjbsbkbbzklylxgwxjjwaqzmywsjqlcj"
	"xxjqwjeqxscwetlzhlyyysdzpyqyzcptlshtzcfy"
	"cyxyljsdcjjagyslcllyyysglrqqeldxzsccccad"
	"ycjysfsgbfrsszqsbxjpsjwsdrckgjlgdkzjzbdk"
	"tcsyqpyhstcldjlhmxmcgxyzhjdctmhltxzxylym"
	"ohyjcltyfbqqjbfbdfehtksqhzywwcnxxcdwhhwg"
	"yjlegmdqcwgfjhcsntwydolbygwqwesjpwnmlryd"
	"zsztxyqpzgcwxhngpyxshmdqjgztdppbfyhzhhjy"
	"fdzwkgkzbldntsxhqeegzxylzmmzyjzgszxkhkht"
	"xexxgylyapsthxdwhzydpxagkydxbhnhxkdfjnmy"
	"hylpmgocslnzhkxxlbzzlbmlsfbhhgsgyyggbhsc"
	"yajtxwlxtzqcwzydqdqmmgdqllszhlsjzwfjhqsw"
	"scelqazynytlsxthaznkzzsdhlacxtwwcsgqqtdd"
	"yzbcchyqzflxpslzygpzsznglydqcbdlxjtctajd"
	"kywnsyzljhhdzcwnyyzyomhychhhxhjkzwsxhdnx"
	"lyscqydpclyzwmypbkxyjlkzhtyhaxqsyshxasmc"
	"hkdscrswjpwqsgzjlwwschs.hsqnhzsngndaqtba"
	"alzzmsstdqjcjktscjaxplggxhhgoxzcxpdmmhld"
	"gtybysjmxhmrcplxjzckzxshflqxccdhxezfchzc"
	"cdytcjyxqhlxdhypjqxnlsyydzozjnyxqezysjya"
	"yjkypdghddxsppyzndlthrhxydpcjjhtcxmctlhb"
	"ynyhmhzllhnxmylllmdcppxhmxdkycyrdltxjchh"
	"znxclcclylnzsxzjzzlnnllwhyqsnjhxynttdkyj"
	"pychhyegkcttwlgqrlggtgtygyhpyhylqyqgcwyq"
	"kpyyyttttlhyhlltyttsplkyzwgywgpydqqzzdqx"
	"skcqnmjjzzbxyqmjrtfbbtkhzkbjdjjkdjjtlbwf"
	"zpbtkqtztgpdgntpjyfalqmkgxbcclzfhzclllla"
	"dpmxdjhlcclgyhdzfgyddgcyyfgydxkssebdhykd"
	"kdkhnaxxybfbyyhxcqgabfqyjjdmljcsjzllpchb"
	"sxgjyndybyqspqwjlzkcddtaccbkzdyzypjzqsjn"
	"kktknjdjgyepgtlfyqkasdntcyhblgdzhbbydmjr"
	"ygkzyheyybcmcdtyfzjjhgcjplxhldwxjjkytcyk"
	"sssmtwcttqzlzbszdtwzxgzagyktywxlhlcpbcll"
	"oqmmzsslcmbjcszzkydczxgqjdsmcytzqqlwzqzx"
	"ssbpkdfqmddzdsddtdmfhtdyzjaqjqkypbdjyyxt"
	"ljhdrqxxxhaydhrjlklytwhllrllrcxylbwsrszz"
	"symkzzhhkyhxksmzsyzgcjfbzbsqlfcxxxnxkxwy"
	"msddyqwggqmmyhcdzttfgyyhgstttybykjdhkyjb"
	"elhdypjqnfxfdqkzhqkzbyjtzbxhfdxbdaswhawa"
	"jldyjsfhbldnndnqjtjnchxfjsrfwhzfmdrfjyhw"
	"zpdjkzyjymfcyznynxfbytfwfwygdbnzzzdnytxz"
	"emmqbsqehxfzmbmflzzsrsymjgsxwzjsprydjsjg"
	"xhjjgljjynzjjxhgjkymlpeyycsysgqzswhwlyrj"
	"lpxslcxmfsmwkcctnxnynpnjszhdzeptxmwywayy"
	"sywlxjqzqxzdclaeelmcpjpclwbxsqhfwrtffjtn"
	"qjhjqdxhwlbyccfjlalkyyjldxhhycstdywncjtx"
	"ywdrmdrqhwqcmfjdyzmhmayxjwmyzqsxtlmrspww"
	"chajbxtgcypxyyrrclmpamgkqjszyjrmyjsnxtpl"
	"nbappypylxmyzkynldgyjzczhnlmzhhanqmpgwqt"
	"zmxxmllhgdzxyhxkrxycjmffxyhjfsbssqlhxndy"
	"cannmtcjcyprrnytyqnyymbmsxndlylysljnlqys"
	"hqmllyzlzjjjkymzcsfbzxxmstbjgnxyzhlsnmcq"
	"scyznfzlxbrnnnylmnrtgzqysatswryhyjzmzdhz"
	"gzdwybsscskxsyhytsxgcqgxzzbhyxjscrhmkkbs"
	"czjyjymkqhzjfnbhmqhysnjnzybknqmcjgqhwlsn"
	"zswxkhljhyybqcbfcdsxdldspfzfskjjzwzxsddx"
	"jseeegjscssmgclxxkywyllymwwwgydkzjgggtgg"
	"sycknjwnjpcxbjjtqtjwdsspjxzxnzxwmelptfsx"
	"tllxcljxjjljsxctnswxlehhlyqrwhsycsqrybya"
	"ywjejqfwqcqqcjqgxaldbzzyjgkgxpltqyfxjltp"
	"adkyqhpmatlcpdhkxmtxybhblefxdleegqdymsaw"
	"hzmljtwyqxlyjzljeeyxbqqffnlyxrdsctgjgxyy"
	"lkllxqkcctlhjlqmkkzgcyygllljdzgydhzwxpys"
	"jbzkdzgyzzhywyfqytyzszyezklymhjjhtsmqwyz"
	"lkyywzcsrkqyqltdxwcdrjklwsqzwbdcqyncjsrs"
	"zjlkcdcdtlzzzacqqczddxyplxcbqjylzllljddz"
	"jgyjyjzyxnyyynxjxkxdazwyrdljyyyrjlglldrx"
	"jcykywnqcclddnyyykyckczhjxcclgzqjgjwppcq"
	"qjysbzzxyjxjbxjfzbsbdsfnsfpzxhdwztdmpptb"
	"lzzbzdmyypqjrsdzsqzsqxbdgcpzswdwcsqzgmdh"
	"zxmwwfybpdgphtmjthzsmmbgzmbzjcfzhfcbbzmq"
	"cfmbcmcjxlgpnjbbxgyhyyjgptzgzmqbqdcgybjx"
	"lwzkydpdymgcftpfxyztzxdzxtgkmtybbclbjask"
	"ytssqyymscxfjeglsllszbqjjjaklyldlycctsxm"
	"cwfgkkbqxlllljyxtyltyxytdpjhnhgnkbyqnfjy"
	"yzbyyessessgdyhfhwtcjbsdzjtfdmxhcnjzymqw"
	"srxjdzjqpdqbbsdjggfbkjbxdgjhmgwjjjgdllth"
	"zhhyyyyyysxwtyyyccbdbpypzyccztjpzywcbdlf"
	"wzcwjdxxhyhlhwczxjtczlcdpxdjczczlyxjjsjb"
	"hfxwpywxzptdzzbdccjhjhmlxbqxxbylrddgjrrc"
	"tttgqsczwmxfytmwzcwjwxjywcskybzqccttqnhx"
	"nkxxkhkfhtswoccjybcmpzzyjbnnzpbthhjdlscd"
	"dytyfjpxyngfxbyqxcbhxcbsxtyzdmzysnxsxlhk"
	"mzxlthdhkghxjsshqyhhcjyxglhzxcsnhekdtgqx"
	"qypkdhextykcnymyyypkqyytjxzlthhqtbyqhxbm"
	"yhsqckwwyllhcyylnneqxqwmcfbdccmljggxdqkt"
	"lxkgnqcdgzjwyjjlyhhqtttnwchhxcxwheszjydj"
	"ccdbqcdgdnyxzdhcqrxcbmztqcbxwgqwyybxhmby"
	"mykdyecmqkyaqyngyzslfykkqgyssqyshjgjcnxk"
	"zycxsbkyxhyylstycxqthysmgscpmmgcccccmtzt"
	"asmgqzjhklosqylswtmqsyqkdzljqqyplcycztcq"
	"qpbbqjzclpkhqcyyxxdtdddsjcxffllchqxmjlwc"
	"jcxtspycxndtjshjwxdqqjckxyamylsjhmlalykx"
	"cyydmamdqmlmcznnyybzkkyflmchcmlhxrcjjhsy"
	"lnmtjggzgywjxsrxcwjgjqhqzdqjdzjjzkjkgdzq"
	"gjjyjylhzxxcdqhhhestmhlfsbdjsyyshfyssczq"
	"lpbdrfrztzdkykgsctgkwdqzrkmsynbcrxqbjyfa"
	"xpzzedzcjykbcjwhyjbqdzywnyszptdkzpfpbazt"
	"klqyhbbzpnbptyzzybhnydcpjmmcycqmcjfzzdcm"
	"nlfpbplngqjtbttajzpzbbdnjkljqylnbzqhksjz"
	"nggqsczkyxchpzsnbcgzkddzqanzgjkdntlzldwj"
	"ljzlywtxndjzjhxyatncbgtzcsskmnjpjytsrwxc"
	"fjwjjtkhtzplbhsnjzsyjbwbzyzlstlsbjhdwwqp"
	"slmmfbjdwajyzccjtbnnrzwqxcdslqgdsdpdzhjt"
	"qqpsqlyyjzlgyhszlctcbjtktyczjtqkbpjlgmjz"
	"dmcsgpynjzjjyyknhrpwszxmtncszzyxybyhyzax"
	"ywkcjtllckjjtjhgcxdxyqyczbywblwqcglzgjgq"
	"rqcczssbcrbcskydznljsqgxssjmecnstztpbdlt"
	"hzwhqwqtzexnqczgweskssbybstscsjccgbfsdqs"
	"zlccglllzghzcthcnmjgyzaznmckcstjmmzckbjy"
	"gqljyjppldxrgzyxccsnhshgdznlzhzjjcddcbcj"
	"flbfqbczzwpqdnhxljcthqwjgylnlszzpcjdscqq"
	"hjqkdxkpbajyemsmjtzdxlcjyryynwjbngzzkmjx"
	"ltbsllrtpylcsznxjhllhyllqqzqlxymrcwcxslj"
	"mczltzldwdjjllnzggqxppskygyggbfzpdkmwghc"
	"xmcgdxjmcjsdycabxjdlnbcddygskydjtxdjjyxm"
	"saqazdzfslqxyjsjzylblxxwxqqzbjzlfbblylwd"
	"sljhxjyzjwtdjcyfqzqzzdcsxzzqlzcdzfchyspy"
	"mpqzmlpplffxjjnzzylsjyyqzfpfzksywjjjhrdj"
	"zzxtxxglghtdxcskyswmmtcwybazbjkshfhgcxmh"
	"fqhyxxyzftsjyzbxyxpzlchmzmbxhzzssyfdmncw"
	"dabazlxktcshhxkxjjzjsthygxsxyyhhhjwxkzxc"
	"sbzzwwhhcwtzzzpjxsnxqqjgzyzawllcwxzfxgyx"
	"yhxmkyyswsqmnjnaycyspmjkgwcqhylajjmzxhmm"
	"cnzhbhxclxtjpltxyjhdyylttxfszhyxxsjbjyay"
	"rsmxyplckdlyhlxrlnllstyzyyqygyhhsccsmcct"
	"zcxhyqfpyyrpfflfqtntszllzmhwtcjqyzwtllml"
	"mdwmbzssmzrbpdddlgjjbxccsrzqqygwcsxfwzlx"
	"ccrbtdzmcyggdlqsgtjmwljmymmsyhfbjdgyxccp"
	"shxczcsbsjwjgjmpbwaffyfnxhydxzylremzgzcy"
	"hsszdlljcsqfzxxkptxzgxjjgbmyyysnbdylbnlh"
	"bfzdcyfbmgqrrmsszxysgtznnydzzcdgbjafjbdk"
	"nzblcsscpsgzycjszlmlrzzbzzldlsllysxsqzql"
	"yxzlsgkbrxbrbzcycxzjzeeyfgklzlyyhgysgzlf"
	"jhgtgwkraajyzkzqtsshjjxdzyz.yjlzyrzdqqhg"
	"jzxsszbtkjpbfrtjxllfqwjgslqtymblpzdxtzag"
	"bdhzzrbgjhwnjtjxlhscfsmwlldqysjtxkzscfwj"
	"lbxftzlljzllqblcqmqqcgcdfpbbhzczjlpyygjd"
	"tgwdcfczqyyyqysrclqzfklzzzgffsqnwglhjycj"
	"jczlqzcyjbjzzbpdccmhjgxdqdgdlzqmfgpsytsd"
	"yfwwdjzjysxyycjcyhzwpbyhxrylybhkjksfxtzj"
	"mmchhlltnyymsxxyzpyjjycdyzwmtjjkqyrhllqx"
	"psgtlwycljscbxjyzfnmlrgjjtyzbsyzmsjyjhgf"
	"zqmsyxrszcwtlrtqzsstkxgqggsptgcdnjsgcqcq"
	"hmxggztqydjkzdlbzsxjlhyqgggthqscpyhjhhgn"
	"ygkggcmjdzllcclxqsftgzslllmlcskctbljzzsz"
	"mmnytpzsxqhjcjyqxyexzqzcpshkzzysxcdfgmwq"
	"rllqxrfztlysdctmjcsjjdhjnxtnrztzfqrhqgll"
	"gcxszsjdjljcytsjtlnyxhszxcgjzyqpylfhdjsb"
	"pcczgjjjqzjqdybssllcmyttmqtbhjqnnygkynqy"
	"qmzgcjkpdcgmyzhqllsllclmholzgdylfzsljcqz"
	"lylzcjeshnylljxgjxlyjyyyxnbcljsswcqqcjyl"
	"lcldjyllzllbnylgqchxyyqoxccqkyjxxhyklksx"
	"ayqccqkkkkcsgyxxyqxygwtjohthxpxxcsshcyey"
	"chzzcbwqbbwjqcscszsslzylgdesjzmmymcytsds"
	"xxscjpqqsqylyfzychdjdzywcbtjsydjhcyddjlb"
	"djjsodzyqysqkxxdhhgqjyohdyxwgmmmajdybbbp"
	"pbcmhcpljzsmtxerxjmhqdstpjdcbssmsssthjts"
	"lmmtrcplzszmlqdsdmjmqpnqdxcfynbfsdqqyxhy"
	"aykqyddlqyyysszbydslntfgtzqbzmchdhczcwfd"
	"xtmqqsphqwwxsrgjcwtjtzzqmgwjjrjhtqjbbgwz"
	"fxjhnqfxxqywyyhyscdydhhqmnmdmmcpbszppzzg"
	"lmzfollcfwhmmsjzttthlmyffytzzgzyskjjxqyj"
	"zqphmbzzlyghgfmshpcfzsnclpbqsnjszslxjfpm"
	"tyjygbxlldlxpzjypjyhhzcywhjylsjexfsszywx"
	"kzjlladtmlymqjpwxxhxsktqjezrpxxzghmhwqpw"
	"qlyjjqjjzszcfhjlchhnxjlqwzjhbmzyxbdhhypy"
	"lhlhlgfwlcfyytlhjjcjmscpxstkpnhjxsntyxxt"
	"estjctlsslstdlllwwyhdhrjzsfgxssyczykwhtd"
	"hwjslhtzdqdjzxxqggyltzphcsqfzlnjtclzpfst"
	"pdynylgmjllycqhynsbchylhqyqtmzybbywrfqyk"
	"jsyslzdyjmpxyyssrhzjnyqtqdfzbwwdwwrxcwhg"
	"yhxmkmyyyhmsmzhngcepmlqqmtcwctmhmxjpjjhf"
	"xyyzsjzhtybmstsyjdtjjqytlhynbyqzlcxcnzws"
	"mylkfjxlwgbypjytysylymzckttwlgsmzsylmpwl"
	"zwxwqzssaqsyxyrhssntsrapccpwcmgdhhxzdzxf"
	"jhgzttsbjhgyglzysmyclllybtyxhbbzjkssdmal"
	"hhycfygmqypjycqxjllljgclzgqlycjcctotyxmt"
	"mshllwcgfxymzmklpszzzxhhjyslctyjcyhxsgyx"
	"zkxlzwpyjpdhjwpjpwsqqxlxxdhmrslzcyzwstcx"
	"kystzshbsccstplwsscjchjlcgchssphylhfhhxj"
	"sxyllnylmzdhzxylsxlwzyhcldyahzcmddyspjtq"
	"jzlngjfsjshctsdszlblmssmnyymjqbjhrcwtyyd"
	"chqljapzwbgqybkfcmjwlzllyylszydwhxpsbcml"
	"jpscgbhxlqhyrljxyswxhxzlldfhlslymjljyfly"
	"jycdrjlfsyzfsllcqyqfgjyhyszlylmstdjcyhbz"
	"llnwlxxygyyhbmgdhxxhhlzzjzxczzzcyqzfnjwp"
	"ylcpkpykpmclgkdgxzggwqbdxzzkzfbxxlzxjtpj"
	"pttbytszzdwslchzhsltjxhqlhyxxxywzyswtmzk"
	"hlxzxzpyhgchkjfsyh.tjrlxfjxptztwhplyxfcr"
	"hxshxkjxxyhzjdxjwylhyhmjdbflkhtxcwhcfwjc"
	"fpqrxqxcyyyjygrpxgscsxngwchkzdxhflxxhjjb"
	"yzwtsxnncyjjymswzjqrmhxzwfqsylzjzgbhynsl"
	"bgttcsebhxxwxyhhxyxnsqyxmlywrgyqlxbbcljs"
	"ylpsytjzyhyzawlhorjmksczjxxxyxchcytryxqj"
	"ddsjfslyltsffyxlmtyjmjjyyyxltzcsxqzlhzxl"
	"wyxzhdnlrxhxjcdyhlbrlmbrllaxksllljlyxxly"
	"crylcjcgjcmtlzllcyzzpzpcyawhjjfybdyyzsep"
	"ckzdqyqpbpcjpdcyzbdbbcyydycnnpjmtmlrmfmm"
	"gwygbsjgygsmdqqqztxmkqwgxllpjgzbqcdjjjfp"
	"kjkcxbljmswmdtqjxldlppbxcwkcqqbfqjczagzg"
	"mykbhyyhzykndkzmbpjyspxthlfpnyygxjdbkxnh"
	"hjhzjxstrstldxskzysybmxjlxyslbzyslhxjpfx"
	"bqnbylljqkygzmcyzzymccsldlhzgwfwyxzmwcxt"
	"ynxjhbyymcysbmhysmydyshqyzchmjjmzcaahcbj"
	"bbhplxtylsxsdjgjdhkxxtxxnbhnmlngsltxmrhn"
	"lxqjxmzllyswqgdlbjhdcgjyqycmhwfwjybbbyjm"
	"jwjmdpwhxqldyapdfxxbcgjspckrssyzjmslbzzj"
	"fljjjlgxzgyxyxlszqyxbexyxhgcxbpldyhwecdw"
	"wcjmbtxchxyqxllxflyxlljlssfwdpzsmyjclmsw"
	"tczbchqekcqbwlcgydblqppqzqfjqdjhymmcxtxd"
	"rmjwrhxcjzclqxdyynhyyhrslsrsywwzjymtltll"
	"gzqcjzyabsckzcjyccqljsqxalmzyyywlwdxzxqd"
	"llqshgpjfjljhjabcqzdjgthhsstcyjlbswzlxzx"
	"rwgldlzrlzqtgsllllzlymxqgdzhgbdbhzpbrlw."
	"xqbpfdwo..whlypcbjcc.dmbzpbzz.cyqxldomzb"
	"lzwpdwyygdstthcsqsccrsssyslfybfntyjszdfn"
	"dpthtzzmbblxlcmyffgtjjqwftmdpjwdnlbzcmmc"
	"tgbdzlqlpyfhsymjylsdchdzjwjcctljcldtljjc"
	"pddpjdsszynndbjlggjzxsxnlycybjjqxcbylzcf"
	"zppgkcxzdzfztjjfjsjxzbnzyjqttyjwhtyczhym"
	"djxttmpxsflzcdwslshxybzgtfmlcjtacbbmgdew"
	"ycyzcdszcyhflyctygwhkjyylsjcxgywjcbhlcsn"
	"ddbtzbsclyzczzssqdllmqyyhfllqllxfdyhabxg"
	"gnywyypllsdldllbjcyxjzmlhljdxyyqytdlllbb"
	"gbfdfbbqjzzmdpjhgclgmjjpgaehhbwcqxaxhhhz"
	"chxyphjaxhlphjpgpzjqcqzgjjzzgzdmqyybzzph"
	"yhybwhazyjhykfgdpfqsdlzmljxjpgalxzdaglmd"
	"gxmwzqytxdxxpfdmmssympfmdmmkxksyzyshdzkj"
	"sysmmzzzmsydnzzczxbmlstmddnmxckjmztyymzm"
	"zzmsshhdccjemxxkljstgwlsqlyjzllsjssdbpmh"
	"nlyjczyhmxxhgzcjmdhxtkgrmxfwmckmwkdcksxq"
	"mmmfzzydkmsclcmpcgmwrpxqpzdsslcxkyxtmlgj"
	"yahzjgzqmcsnxyhmmpmlkjxmhlmlgmxctkzmjjys"
	"zjsyszhsyjzjcdajzybsdqjzgwzkgxfkdmsdjlfm"
	"ehkzqkjbeypzyszcdwyjffmzjykttdzzefmzlbnp"
	"plplpbpszalltylkckqzkgenqlwagxxydpxlhsxq"
	"qwqykxqclhyxxmlyccwlymqyskychlcjnszkpyzk"
	"cqzqljbdmdjhlasqlbydwqlwdnbqcrydddtjybkb"
	"wszdxdtnpjdtctqdfxqqmgnseclstbhpwslctxxl"
	"pwydzklzygzcqapllkccylbqmqczqcljslqzdjxl"
	"dthpzqdljjxzqdjyzhkzljcyqdyjppypeakjyrmp"
	"cbymcxkllzllfqpylllmbsglcysslrsysqtmxyxq"
	"qzbdzrysyztffmzzsmzqhzssccmlyxwtpzgxzjgz"
	"gsjsgkddhtqggzllbjdzlcbzhyxyzhzfywxyzyms"
	"dbzzyjgtsmtfxqyxjscdgslnmdlrytzlryylxqht"
	"xsrtzcgyxbnqqzfhykmzjbzymkbpnlyzpblmcnqy"
	"zzzsjzhjctzhhyzzjrdyzhnfxglfxslkgjtctssy"
	"llgzrzbbjzzklpkbczyslxyxbjfpnjzzxcdwxzyj"
	"xzzdjjgggrsrjkmcmzjlsjywqshyhqjsxpjzzzls"
	"nshrnypjtwchklbsrzlcxwjqxqkysjycztlqzybb"
	"ybwzjqdwgyzcytjcjxckcwdkkzxsgkdzxwwyyjqy"
	"ytcytdjlxwkczkklcclzcqqdzlqlcsfqchqhsfsm"
	"qzzllbjjzbsjhtsjdysjqjpdlzcdcwjkjzzlpycg"
	"mzwdjjbsjqzsyzyhhxcbbjydssddzncglqmbtsfc"
	"bfdzdlznfgfjgfsmptjqlmblgqcyyxbqkdxjqsrf"
	"kztjdhczklbsdzcfytplljgjhtxzcsszzxstcygk"
	"gckgyoqxjplzbbbgtgyjdgczqszlbjlsjfzgkqqj"
	"cgyczbzqtldxrjxbsxxpzxhyzyclwdsjjhxmfczp"
	"fzhqhqmqgkslyhtycgfrzgnqxclpdlbzcsczqllj"
	"blhbdcypczppdymtzsgyhckcpzjgslclnscdsldz"
	"xbmsdlddfjmkdjdhslzxlszqpqpgjllybdszgqlb"
	"zlslkyyhzttncjyqtzzfszqztlljtyyllqllqyzq"
	"lbdzlslyyzymdfszsnhlxznczqzbbwskrfbcyzmt"
	"hblgjpmczzcstlxshtzcyzlzblfeqhlxflcjlylj"
	"qcbzlzjghsstbrmhxzhjzclxfnbgxgtqjcztmsfz"
	"kjmssnxljkbhszxntnlzdntlmsjxgzjyjczxyhyh"
	"wrwwqnztnfjscpzshzjfyrdjsfscjzbjfzqzchzl"
	"xfxsbzqlzsgyftzdcszxzjbqmszkjrhxjzcgbjkh"
	"chgtjkjqglxbxfgdrtylxjxgdtsjxhjzjjcmzlcq"
	"sbtxhqgxttxhxftsdkfjhzyjfjxrzcdlllcqsqqz"
	"qwqxswqtwgwbzcgcllqzbclmqqtzgzxzxljfrmyz"
	"flxysqxxjkxrmjdcdmmyxbsqbhgcmwfwtgmxlzby"
	"ytgzyccdxyzxswgxyjyznbgpzjcqsyxcxrtfycgr"
	"hztxszzthcbfclsyxzljqmzlmplmxzjssflbysmy"
	"qhxjsxrxsqzzzsslyflczjrcrxhhzxqydshxsjjh"
	"zcxjbdynsysxjbqlpxzqpymlxzkyxlxcjlcycrxz"
	"zlldlllsjyhzxgyjwkjrwyhcpsgnrzlfzwfzznsx"
	"gxflzsxzzzbfcsyjdbrjkrdhhgxjljjtgxjxxstj"
	"tjxlyxqfcsgswmsbctlqzzwlzzkxjmltmjyhsddb"
	"xgzhdlbmyjfrzfcgclyjbpmlysmsxlszjqqhjzfx"
	"gfqfqbpxzgyyqxgztcqwyltlgwwgwhllfsfgzjmg"
	"mgbgtjfsyzzgzyzaflsspmlbflcwbjzcljjmzlpj"
	"jlymqdmyyyfbgygqzglyzdxqyxrqqqhsxyyqqygj"
	"tyxfsfsllgnqcygycwfhcccfxbylypllzqxxxxxk"
	"qhhxshjdcfdsczjxcpzwhhhhhapylhalpqafyhxd"
	"yllkmzqgggddesrnndltzgchybpysqjjhclljtol"
	"njpzljlhymheydydsqycddhgzpndzclzywllznte"
	"ytgxlhslpjjbdgwxpcdntjcklkclwkllcasstknz"
	"dnqnttlyyzssysszzryljqkcgbhhcrxrzydgrgcw"
	"cgzhfffppjfzynakrgywyqpqxxfkjtszzxswzddf"
	"bbqtbgtzfznpzfpzxzpjszbmqhkcyxyldkljnypk"
	"yghgdcjxxeahpnzgctzcmxcxmmjxnkszqnmnlwbw"
	"wxjjyhclstmcsqdjcxxtpcnpdtnnpglllzcjlspb"
	"lplkcdtnjnlyyrscffjfqwdpgzdwmnzcclodaxns"
	"snyzrestyjwjyjdbcfxnmwttbqlwstszgybljpxg"
	"lboclgpcbjftmxzljylzxcltpnclcgxtfzjshcrx"
	"sfyszdkntlbyjcyjllstgqcbxnwzxbxklylhzlqz"
	"lnzcqwgzlgzjncjgcmnzzgjdzxtzjxycyycxxjyy"
	"xjjxsssjstssttppghtcsxwzdcsyfptfbchfbblz"
	"jclzzdbxgcxlqpxkfzflsyltywbmnjhskbmddbcy"
	"sccldxycddqlyjjhmqllcsgljjsyfpyyccyltjan"
	"tjjpwycmmgqyysqdhqmzhszxpftwwzqswqrfkjlx"
	"jqqyfbrxjhhfwjgzyqacmyfrhcyybyqwlpexcczs"
	"tyrltsdmqlykmbbgmyyjprknnbbsxyxbhyzdjdng"
	"hpmfsgbwfzmfjmmbcmzzcjjlcnyxyqgmlrygqccy"
	"hzlwjgcjcggmcjjfyzzjhycfrrcmtzqzxhfqgdjx"
	"ccjeaqcrjthpljlszdjrbcqhjdzrhxlyxjsymhzy"
	"dwldfryhbbydtssccwbxglpzmlzztqsscpjmmxjc"
	"sjytycghycjwsnsxlfemwjnmkllswtxhyyygcmmc"
	"wjdqdjzglljwjnkhpzggflccsczmcbltbhbqjxqd"
	"jpdjqtghglfqawbzyjjltstdhqhctcbchflqmpwd"
	"shyytqwcnztjtlbypbpdyyyxsqkxwyyflxxncwcx"
	"ybmaelykkjmzzzbrxyaqjfljpfhhhytzzxrgqqmh"
	"spgdzjwbwpjhzjdyscqwzkthxsqlzyymysdzgrxc"
	"kkhjlwpysyscsyzlrmlqsyljxbcxtlhdqzpcycyk"
	"pppnsxfyzjjrcemhszmsxlxglrwgcstlrsxbygbz"
	"gztcpldjlslylymdtmtcpalcxpqjcjwtcyyzlblx"
	"bzlqmyljbghdslssdmxmbdczsxwhamlczcpjmcnh"
	"jyjnsygchskqmzzqdllkablwjqsfmocdxjrrlyqc"
	"hjmybyqlrhetfjzfrfksryxfjdwdsxxlwsqjysly"
	"xwjhsnlxyyxhbhawhhjcxwmyljcsqlkydttxbzsx"
	"fdxgxsjhhsxxybssxdpwncmrptjzczenygcxqfjx"
	"kjbdmljcmqqxloxslyxxlylljdzbtymhbfsttqqw"
	"lhogyblscalzxqlhtwrrqhlstmypyxjjxmqsjpnb"
	"ryxyjllyqylthylqyfmhkljdmllhfzwkzhljmlhl"
	"jkljstlqxylmbhhlnlsxqchxcfxxlhyhjjgbyzzk"
	"bxscqdjqdsxjzsyhzhhmgsxcsymxfebcqwwrbpyy"
	"jqtyqcyjhqqzyhmwffhgzfrjfcdbxndqyzpcyhhj"
	"lfrzgppxzdbbgzqstlgdgylcqmgchhmfywlzyxkj"
	"lypqhsywmqqgqzmlzjnsqxjqsyjtcbehsxfssfxz"
	"wfllbcyyjdytdthwzsfjmqqyjlmqsxlldttkhhyb"
	"fpwdyysqqrnqwlgwdebdwcyygcdlkjxtmxmyjsxh"
	"ybrwfymwfrxyqmxysctzztfykmldhqdlwyqnlcry"
	"jblpsxcxywlsbrrjwxhqybhtydnhhgmmywytzcsq"
	"mtssccdalwztcpqpyjllqzyjswxwzzmmglmxclmx"
	"czmxmzsqtzppjqblpgxjzhfljjhycjsnxwcxsccd"
	"lxsyjdcqcxslqyclzxlzzxmxqrjmhrhzjphmfljl"
	"mlclqnldxzlllfypngjysxcqqdcmqjzzxhnpnxzm"
	"ekmxxykyqlxsxtxjxyhwdcwdzhqyybgybcyscfgf"
	"sjnzdyzzjzxrzrqjjymcanhrjtldbpyzbstjhxxz"
	"ypbdwfgzzrpymtngxzqbyxmbbfcckrjjjbjegrzg"
	"yclkxzdxkknsjkcljspgyyzlqqjybzssqlllkjfc"
	"bktylcccdblsppfylgydtzjyqggkqttfcxbdkdxx"
	"hybbfytyhbclpdytgdhryrnjsbtcsnyjqhklllzs"
	"lydxxwbcjqsbxbfjzjcjdzfbxxbrmlazgcsnclbj"
	"dstblfrzdswsbxbcllxxlzdjzsjpylyxxyftfffb"
	"hjjjgbygjpmmmmsscljmtlyzjxswxtyledqpjmyg"
	"qzjgdjlqjwjqllsdgjgygmscljjxdtygjqjqjcjz"
	"cjgdzdshqgsjggcjhqxsnjlzzbxhsgzxcxyljxyx"
	"yydfqqjhjfxdhctxjyrxysqtjxyefyyssyxjxncy"
	"zxfxcsyszxyyschshxzzzgzzzgfjdldylnpzgyjy"
	"zyyqzpbxqbdztzczyxxyhhscxshcggqhjhgxwszt"
	"mzmehyxgebtylzkkwytjzrclekestdbcykqqsayx"
	"cjxwwgsbhjszsdhcsjkqcxswxfctynydpzcczjqt"
	"zwjqdzzzqzljchlsbhpydxpsxshhezdxfptjqyzz"
	"xhyaxncfzyyhxgnqmywxtzsjpkhhgymxmxqcxtsb"
	"cqsjyxhtyylybcqlmmszmjzjllcogxzaajzyhjmc"
	"hhcxzsxzdznleyjjzjbhzwzzsqtzpsxztdsxjjjz"
	"nyazphhyysrnqdthzhayjyjhdzxzlswclybzyecw"
	"cycrylcxnhzydzydyjdfrjjhtrsqtxyxjrjhojyn"
	"xelxsfsfjzghpzsxzszdzcqzbyyklsgsjhczshdg"
	"qgxyzgxchxzjwyqwgyhksseqzzndzfkwyssdclzs"
	"tsymcdhjxxyweyxczaydmpxmdsxybsqmjmzjmtzq"
	"lpjyqzcgqhxjhhhxxhlhdldjqsldwbsxfzzyysch"
	"tytyjbhecxhjkgjfxbhyzjfxbwhbdzfyzbcapnpg"
	"nydmsxhkhhmamlnbyjtmpxyjmcthjbzyfcgtyhwp"
	"hftgzzezsbzegpbmdskftycmhbllhgpzjxzjgzjy"
	"xzsbbqsczzlzccstpgxmjsftcczjzdjxcybzlfcj"
	"syzfgszlybcwzzbyzdzypswyjgxzbdsysxlgzbzf"
	"ygczxbzhzftpbgzgejbstgkdmfhyzzjhzllzzgjq"
	"zlsfdjsscbzgpdlfzfzszyzyzsygcxsntxchczxt"
	"zzljfzgqsqyxzjqccccdjcdxzjyqjccgxztdlgsc"
	"xzsyjjqtcclqdqztqchqqjztezzzpbkkdjfcjfzt"
	"ybqyqttynlmbdktjcpqzjdzfpjsbnjlgyjdxjdzq"
	"kzgqkxclpzjtcjdqbxdjjjstcjnxbxcmslyjcqmt"
	"jqwwcjjnjnlllhjcwqtbzqyczczpzzdzyddcyzdz"
	"ccjgtjfzdprntctjdcqtqndtjnplzbcllctdsxkj"
	"zqdpzlbznbtjdcxfczdbccjjltqjpldcgzdbbzjc"
	"qdcjwynllzlzccdwllxwzlxrsntqjccxkjlsgdfq"
	"tddglrlajjtklymkqlldzytdyycygjwyxdxfrsks"
	"tcdenqmrkqzhhqkdldazfkypbggpzrebzzykyzsp"
	"egjjglkqzzzslysywyzwfqznlzzlzhwcgkypqgnp"
	"gblplrrjyxcccgyhsfzfwbzywtgzxyljczwhxzjz"
	"blfflgskhyjzeyjhlpllllcygxdrzelrhgklzzyh"
	"zlyqszzjzqljzflnbhgwlczcfjwspyxnlzlxgccp"
	"zbllcxbbbbxbbcbbcrnncccyrbbsyldcgqyyqxyg"
	"mqzwtzydyjhyfwdehzdjywlccntzyjjcdedpzdzt"
	"stqjhdymbjnyjzlxtsstphndjxxbyxqtzqddtjtd"
	"yztgwscszqflshlglbcjbhdlyzjyckwtydylbnyd"
	"sdsycctyszyyebgexhqddwnygyclxtdcystqmygz"
	"asccszzddlcclzrqxyyeljsbymxshztembbllyyl"
	"lytdqyshymrqxkfkbfxnxsbychxbwjyhtqbpbsbw"
	"dzylkgzskyghqzjhhxjxgnljkzlyycdxlfwfghlj"
	"gjybxblybxqpqgztzplncybxdjyqydymrbesjyyh"
	"kxxstmxrczzywxyhybmcflyzhqyzmqxdbxbzwzms"
	"lpdmyckfmzklzcyjycclhxfzlydqzpzygyjyzmzx"
	"dzfyfyttqtchgspczmlccytzxjcytjmkslpzhysn"
	"wllytpzctzzcktxdhxxtqcypksmqccyyazhtjpcy"
	"lzlyjbjxtfnyljyynrxcylmmnxjsmybcsysslzyl"
	"ljjgyldzdpqbfzzblfndsqkczfhhhgqmrdsxycst"
	"xnqqjpyjbfcxdyqfbnxejdgyqbsrcnfyyqpghyjd"
	"yzxgrhtkyleqdzntsmgklbsgbpyszbytjzsszjcs"
	"sxzbhbscsbzczptqfzmqflypybbjgszmxxdjmthy"
	"skkbjtxhjcegbsmjyjzcxtmljyxrzzqscxxqptzx"
	"mkyxxxjcljprmyygadyskqlsadhrskqxzxztcghz"
	"tlmlwxybwsycdbhjhcfcwzsxhytkzlxqshlyczjx"
	"emplprcgltbzztlzjcyjgdtclglpllqpjmzpapxy"
	"zlaktkdnczzbnzctdqqzjyjgmctxltgcszlmlhbg"
	"lkfwnwzhdxphlfmkydlgxdtwzfrjejctzhydxyks"
	"hwfzcqshktmqqhtchymjdjskhxzjzbzzxympajqm"
	"sdbxlsklyynwrtsqlscbpdbsgzwyhtlkssswhzzl"
	"yytnxjgmjszsxfwnlsoztxgxlsammlbwldszylak"
	"qcqctmycfjbslxclzjclxxksbzqclhjphqplsxsc"
	"kslnhpsfqqytxjjzlqldxzjjzdyydjnzptfcdskj"
	"fsljhylzqjzlbthydgdjfdbyazxdzhzjnhhqbykn"
	"xjjqczmlljzkspldsclbblxklelxjlbjycxjxgcn"
	"lcqplzlznjtzljgyzdzpltqcssfdmnycxgbtjdcz"
	"nbgbqyqjwgkfhtnbyqzqgbkpbbyzmtjdytblsqmb"
	"sxtbnpdxklemyycjynzdtldykzzxddxhqshdgmzs"
	"jycctayrzlpwltlkxslzcggexclfxlkjrtlqjaqz"
	"ncmbqdkkcxglczjzxjhptdjjmzqykqsecqzdshha"
	"dmlzfmmzbgntjnnlgbyjbrbtmlbyjdzxlcjlpldl"
	"pcqdhlhzlycblcxzcjadqlmzmmsshmybhbskkbhr"
	"sxxjmxsdznzpxlbbragggfchgmsklltsjyycqlcs"
	"kywyehywhbhqywbawykqldqftntkhqcgdqktgpkx"
	"hcpdhtwtmssyhbwcrwxhjmkmzngwtmlkfghkjyld"
	"yycxwhyeclqhkqhtdqhhffldxqwgzyydesbpkyrz"
	"pjfyyzjceqdzzdlattbbfjllcxdlmjsdxegygsjq"
	"xcfbxsszpdyzcxdnyxpfzydlyjccpltxlsxyzyrx"
	"cyysdylwwndsahjsygyhgywkaxtjzdaxysrltdjs"
	"saxfnejdxyzhlxlllzhzsjnyqyqqxyjghzgjcyjc"
	"hzlycdshwsgczyjxcllnxzjjyyxnfsmwfpylcyll"
	"abwddhwdxjmcxztzpmlqzhsfhzynztlldywlslxh"
	"ymmylmbwwkyxyadtxylldjpybpwfxjmmmllhafdl"
	"laflbhhhbqqjtzjcqjjdjtffkmmmbythygdcqrdd"
	"wrqjxnbysnmzdbyytbjhpybygtjxaahgqdqtmyst"
	"qxkbtsbkjlxrbeqqhqmjjbdjwtgtbxpgbktlgqxj"
	"jjcdhxqdwjlwrfmqgwqhckryswgbtgygbwsdwdwr"
	"fhwytjjxxxjyzyslphyypayxhydqkxshxyxeskqh"
	"ywbdddpplcjlhqeewxksyyhdyplfjthkjltcyyhh"
	"jttpltzzcdlthqkcxqysteeywkyzyxxyysddjkll"
	"pwmcyhqgxyhcrmbxpllnqydqhxsxxwgdqbshyllp"
	"jjjthyjkyphthyyktyezyenmdshlcrpqfbgfxzbs"
	"btlgxsjbswyysksflxlpplbbblbsfxfyzbsjssyl"
	"pbbffffsscjdstzsxtryjcyffsytyzbjtbctsbsd"
	"hrtjjbytcxyjeylxcbnebjdsysyhgsjzbxbytfzw"
	"genyhhthjhatfwgcstbgxklstywmtmbyxjskzscd"
	"yjrcytwxzfhmymcxlznsdjtttxrycfyjsbsdyerx"
	"hljxbbdeynjghxgckgscymblxjmsznskgxfbnbbt"
	"hfjaafxyxfpxmyfhdtzcxzzpxrsywzdlybbjtyqw"
	"qjpzypzjznjpzjlztfysbttslmptzrtdxqsjehbz"
	"ylzdhljsqmlhtxtjecxalzzspktlzkqqyfsygywp"
	"cpqfhqhytqxzkrsgtgsqczlptxcdyyzssqzslxlz"
	"macbcqbzyxhbsxlzdltcdjtylzjyytpzylltxjsj"
	"xhlbmytxcqrblzssfjzztnjydxmyjhlhpblcyxqj"
	"qqkzzscpzkswalqsblcczjsxgwwwygyatjbbctdk"
	"hqhkgtgpbkqyslbxbbckbmllxdzstbklggqkqlsb"
	"kkdfxrmdkbftpzfrtbbmferqgxkjpzsstlbzdpsz"
	"qzsjthljqlzbpmsmmsxlqqnhknblrddnhxdhddjc"
	"yygyfqgzlgsygmjqgkhbpmxyxlytqwlwgcpbmjxc"
	"yzydrjbhtdjxeeshtmjsbyplwhlzffnypmhxqhpl"
	"tbqpfbcwjdbygpnxtbfzjgsddtjshxeawzzylltt"
	"ybwjkgxghlfkxdjtmszsqynzggswqsphtlsskmcl"
	"zxyszqzxncjdqgzdlfnykljcjllzlmzznhydssht"
	"hxzlzzbbhqzwwycrdhlyqqjbeyfsgxthsrxwqhwf"
	"slmssgzttyeyqqwrslalhmjtqjsmxqbjjzjxzyzk"
	"xbyqxbjxshzssfglxmxzxfghkzszggylclsarjxh"
	"slllmzxelglxydjytlfbhbpnlyzfbbhptgjkwetz"
	"hkjjxzxxglljlstgshjjyqlqzfkcgnndjsszfdbc"
	"twwseqfhqjbsaqtgypjlbxbmmywxgslzhglzgnyf"
	"ljbyfdjfrgsfmbyzhqfbwjsyfyjjphzbyyzffwod"
	"grlmftmlbzgycqxcdjygzyyyytytydwegazyhxjl"
	"zythlrmgrjxzclhneljjthtbwjybjjbxjjtjteek"
	"hwsljplpsfazpqqbdlqjjtyyqlyzkdksqjyyjzld"
	"qcgjjyzjsycmraqthtejmfctyhypkmhycwjdcfhy"
	"yxwshctxrljgjshccyyyjltkttytmxgtcjtzayyo"
	"czlylbszywjytsjyhbyshfjlygjxxtmzyyltxxyp"
	"clxyjzyzyypnhmymdyylblhlsyygqllnjjymsoyc"
	"bzgdlyxylcqyxtszegxhzglhwbljgeyxtwqmakbp"
	"qcgyshhegqcmwyywljyjhyyzlljjylhzyhmgsljl"
	"jxcjjyclycjpcpzjzjmmylcjlnqljjjlxxjmlszl"
	"jqlycmmhcfmmfpqqmfxlqmcffqmmmmhmznfhhjgt"
	"thhkhslnchhyqdxtmmqdcydyxyqmyqylddcyyyda"
	"zdcymzydlzfffmmycqcwzzmabtbyctdmndzggdft"
	"ypcgqyttssffwbdtzqssystwjjhjytsxxylbyqhw"
	"whxezxwznnqzjzjjqjccchyyxbzxccyjtllcqxkn"
	"jycyycynzzqyyoewyczdcjycchyjlbtzkycqwlpg"
	"pyllgkdldlgkgqbgychjxy....z.w...z....s.."
	"...l.......sl..z....lgtm....gd.........."
	"...........atn..hbltdzlatmmbj.lx";

int Pinyin_decode(const char* s, uint32_t* cp) {
	const unsigned char* p = (const unsigned char*)s;
	if (p[0] < 0x80) {
		*cp = p[0];
		return 1;
	}
	if ((p[0] & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) {
		*cp = ((uint32_t)(p[0] & 0x1F) << 6) | (p[1] & 0x3F);
		return 2;
	}
	if ((p[0] & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
		*cp = ((uint32_t)(p[0] & 0x0F) << 12) | ((uint32_t)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
		return 3;
	}
	if ((p[0] & 0xF8) == 0xF0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80 && (p[3] & 0xC0) == 0x80) {
		*cp = ((uint32_t)(p[0] & 0x07) << 18) | ((uint32_t)(p[1] & 0x3F) << 12) | ((uint32_t)(p[2] & 0x3F) << 6) | (p[3] & 0x3F);
		return 4;
	}
	*cp = p[0];
	return 1;
}

char Pinyin_initial(uint32_t cp) {
	if (cp < PINYIN_START || cp >= PINYIN_START + PINYIN_COUNT)
		return 0;
	char c = PINYIN_DATA[cp - PINYIN_START];
	return c == '.' ? 0 : c;
}

bool Pinyin_initials(const char* utf8, char* out, size_t out_size) {
	if (!utf8 || !out || out_size == 0)
		return false;
	size_t o = 0;
	for (const char* s = utf8; *s;) {
		uint32_t cp;
		int n = Pinyin_decode(s, &cp);
		s += n;
		char c = 0;
		if (cp < 0x80) {
			if (cp >= 'A' && cp <= 'Z')
				c = (char)(cp - 'A' + 'a');
			else if ((cp >= 'a' && cp <= 'z') || (cp >= '0' && cp <= '9'))
				c = (char)cp;
		} else {
			c = Pinyin_initial(cp);
		}
		if (c && o + 1 < out_size)
			out[o++] = c;
	}
	out[o] = '\0';
	return o > 0;
}

size_t Pinyin_sortKey(const char* utf8, char* out, size_t out_size) {
	if (!out || out_size == 0)
		return 0;
	size_t o = 0;
	for (const char* s = utf8 ? utf8 : ""; *s;) {
		uint32_t cp;
		int n = Pinyin_decode(s, &cp);
		char c = cp < 0x80 ? (char)((cp >= 'A' && cp <= 'Z') ? cp - 'A' + 'a' : cp) : Pinyin_initial(cp);
		if (c) {
			if (o + 1 >= out_size)
				break;
			out[o++] = c;
		} else {
			if (o + (size_t)n >= out_size)
				break;
			for (int i = 0; i < n; i++)
				out[o++] = s[i];
		}
		s += n;
	}
	out[o] = '\0';
	return o;
}

bool Pinyin_hasMultibyte(const char* s) {
	for (; *s; s++)
		if ((unsigned char)*s >= 0x80)
			return true;
	return false;
}

#endif // PINYIN_IMPL_DONE
#endif // PINYIN_IMPL
