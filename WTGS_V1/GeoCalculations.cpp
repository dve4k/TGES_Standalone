#include "stdafx.h"
#include "GeoCalculations.h"

//#pragma optimize("", off)

double firFilter(int coeffIndex, double *data, int windowWidth);
bool putCompleteCalcPacket_outputBuffer(CoreGeomCalculationPacket p);
bool putCompleteCalcPacket_outputBuffer(CoreGeomCalculationPacket p);
double meanCalc(int coeffIndex, double *data, int halfWidth1, int halfWidth2);
double medianCalc(int coeffIndex, double *data, int halfWidth);
double fraMeanCalc(int coeffIndex, double *data, int halfWidth1, int halfWidth2, int cordLength);
bool chainExceptionsAndShift(WTGS_Exception* newExceptions, WTGS_Exception* oldExceptionBuffer, int &newExceptionCount, int &oldExceptionCount, WTGS_Exception* completeExceptions, int &completeExceptionCount);
bool putExceptionOutputQue(WTGS_Exception exception);

//ORIGINAL CURAVTURE 141 TAP FIR FILTER
//double firCoeff_1[] =
//{
//	-0.0001600033,
//	-0.0001437833,
//	-0.0001279283,
//	-0.000111781,
//	-9.46E-05,
//	-7.57E-05,
//	-5.42E-05,
//	-2.93E-05,
//	7.67E-20,
//	3.45E-05,
//	7.52E-05,
//	0.0001229414,
//	0.0001788008,
//	0.0002436905,
//	0.0003185562,
//	0.000404326,
//	0.000501903,
//	0.0006121567,
//	0.0007359154,
//	0.0008739582,
//	0.0010270071,
//	0.00119572,
//	0.0013806832,
//	0.001582405,
//	0.001801309,
//	0.0020377288,
//	0.002291902,
//	0.002563966,
//	0.0028539537,
//	0.0031617901,
//	0.0034872893,
//	0.0038301528,
//	0.0041899678,
//	0.0045662071,
//	0.0049582288,
//	0.0053652779,
//	0.0057864876,
//	0.006220882,
//	0.0066673798,
//	0.0071247979,
//	0.0075918567,
//	0.0080671855,
//	0.0085493286,
//	0.0090367526,
//	0.0095278534,
//	0.0100209648,
//	0.0105143667,
//	0.0110062944,
//	0.0114949479,
//	0.0119785017,
//	0.0124551147,
//	0.0129229408,
//	0.0133801392,
//	0.0138248845,
//	0.0142553777,
//	0.0146698562,
//	0.0150666045,
//	0.0154439636,
//	0.0158003414,
//	0.0161342218,
//	0.0164441737,
//	0.0167288599,
//	0.0169870447,
//	0.0172176014,
//	0.0174195193,
//	0.0175919098,
//	0.0177340114,
//	0.0178451949,
//	0.0179249668,
//	0.0179729724,
//	0.0179889981,
//	0.0179729724,
//	0.0179249668,
//	0.0178451949,
//	0.0177340114,
//	0.0175919098,
//	0.0174195193,
//	0.0172176014,
//	0.0169870447,
//	0.0167288599,
//	0.0164441737,
//	0.0161342218,
//	0.0158003414,
//	0.0154439636,
//	0.0150666045,
//	0.0146698562,
//	0.0142553777,
//	0.0138248845,
//	0.0133801392,
//	0.0129229408,
//	0.0124551147,
//	0.0119785017,
//	0.0114949479,
//	0.0110062944,
//	0.0105143667,
//	0.0100209648,
//	0.0095278534,
//	0.0090367526,
//	0.0085493286,
//	0.0080671855,
//	0.0075918567,
//	0.0071247979,
//	0.0066673798,
//	0.006220882,
//	0.0057864876,
//	0.0053652779,
//	0.0049582288,
//	0.0045662071,
//	0.0041899678,
//	0.0038301528,
//	0.0034872893,
//	0.0031617901,
//	0.0028539537,
//	0.002563966,
//	0.002291902,
//	0.0020377288,
//	0.001801309,
//	0.001582405,
//	0.0013806832,
//	0.00119572,
//	0.0010270071,
//	0.0008739582,
//	0.0007359154,
//	0.0006121567,
//	0.000501903,
//	0.000404326,
//	0.0003185562,
//	0.0002436905,
//	0.0001788008,
//	0.0001229414,
//	7.52E-05,
//	3.45E-05,
//	7.67E-20,
//	-2.93E-05,
//	-5.42E-05,
//	-7.57E-05,
//	-9.46E-05,
//	-0.000111781,
//	-0.0001279283,
//	-0.0001437833,
//	-0.0001600033
//};


//UPDATED CURVATURE 141 TAP FIR FILTER 6/8/2019 -- WAS BASED OFF CURVATURE + RUNNING AVG APPLIED
//double firCoeff_1[] =
//{
//0.039081789668528973957783989590097917244,
//0.003041783838773758508883249263021752995,
//0.003156409531300077061372055808874392824,
//0.003272952424774921813266370662631743471,
//0.003389679717524375027887240108270816563,
//0.003508437944578776408049991530901934311,
//0.003626361270307574755089596862944745226,
//0.003746823973938770164365807957551623986,
//0.003867157873719694223535459087770504993,
//0.00398785583312625657032013393177294347,
//0.004111358040218498523243884079647614271,
//0.004235512789095543552397860054270495311,
//0.004359004061052089255834385284060772392,
//0.004482391038819707497486977132439278648,
//0.004606298261030873988919331907254672842,
//0.004733858801461210408234592250664718449,
//0.004858348426887726737022354939199431101,
//0.004981422426134814174236709050092031248,
//0.00510803257835020299570949475764791714,
//0.005229788985673488113659068687866238179,
//0.00535440314116647237546731119550713629,
//0.005474782961579140183605574065950349905,
//0.005596675733755709578054471364794153487,
//0.005719388384369038930143958054941322189,
//0.005834819045240814336383827765075693605,
//0.0059570619206725005170821596323094127,
//0.006079992293847576688425871083154561347,
//0.006199276002235074196911046584546056692,
//0.006319504708542874639454201002308764146,
//0.006437488248779526384124594073909975123,
//0.006558840329598367505092859630622115219,
//0.006673899129950474490291512097428494599,
//0.006787641275623574818598449098772107391,
//0.006892770298782699422979192149796290323,
//0.006998112915905718364739662717965984484,
//0.007093605843212404636621837283882996417,
//0.007202771807295708754448337884923603269,
//0.007306497807758614675133213012259147945,
//0.007445844906376225882937980316000903258,
//0.007511324821726215496264789095448577427,
//0.007598292863279205479332123474023319432,
//0.007716850054650810711298625221843394684,
//0.007794255624836702277258293491968288436,
//0.007894952045364682716166448983585723909,
//0.007974337216140110762285786449865554459,
//0.00806665151913044282949716290431751986,
//0.008144041309962090149920932447002996923,
//0.008228151555184075316429215263269725256,
//0.0083021130970391994463586371466590208,
//0.008381193535806916519725895398096326971,
//0.008446986892690707890429813176069728797,
//0.008517932831901952467346106345758016687,
//0.008578436571423904768995960523625399219,
//0.008644872064714904935645023442702949978,
//0.008699109927098866434924673285422613844,
//0.008756391623879943020281046983654960059,
//0.008807083356304909460554597444570390508,
//0.008859474339361638611789473429780628067,
//0.008902597260191205327806152070024836576,
//0.008949894599506957870715950775775127113,
//0.008983770520472495260677625594780693064,
//0.009028310722953522896028211164320964599,
//0.009046160957116661238686106116801965982,
//0.00908147299514069569759122657615080243,
//0.009108034196167309054503746779118955601,
//0.009124718362314050701389334108171169646,
//0.009136801608223193982283305558667052537,
//0.009151040100933226228985262196147232316,
//0.009161954628665319386171539406404917827,
//0.009172101490139226293019092395297775511,
//0.009172672526749641938592638723548589041,
//0.009172101490139226293019092395297775511,
//0.009161954628665319386171539406404917827,
//0.009151040100933226228985262196147232316,
//0.009136801608223193982283305558667052537,
//0.009124718362314050701389334108171169646,
//0.009108034196167309054503746779118955601,
//0.00908147299514069569759122657615080243,
//0.009046160957116661238686106116801965982,
//0.009028310722953522896028211164320964599,
//0.008983770520472495260677625594780693064,
//0.008949894599506957870715950775775127113,
//0.008902597260191205327806152070024836576,
//0.008859474339361638611789473429780628067,
//0.008807083356304909460554597444570390508,
//0.008756391623879943020281046983654960059,
//0.008699109927098866434924673285422613844,
//0.008644872064714904935645023442702949978,
//0.008578436571423904768995960523625399219,
//0.008517932831901952467346106345758016687,
//0.008446986892690707890429813176069728797,
//0.008381193535806916519725895398096326971,
//0.0083021130970391994463586371466590208,
//0.008228151555184075316429215263269725256,
//0.008144041309962090149920932447002996923,
//0.00806665151913044282949716290431751986,
//0.007974337216140110762285786449865554459,
//0.007894952045364682716166448983585723909,
//0.007794255624836702277258293491968288436,
//0.007716850054650810711298625221843394684,
//0.007598292863279205479332123474023319432,
//0.007511324821726215496264789095448577427,
//0.007445844906376225882937980316000903258,
//0.007306497807758614675133213012259147945,
//0.007202771807295708754448337884923603269,
//0.007093605843212404636621837283882996417,
//0.006998112915905718364739662717965984484,
//0.006892770298782699422979192149796290323,
//0.006787641275623574818598449098772107391,
//0.006673899129950474490291512097428494599,
//0.006558840329598367505092859630622115219,
//0.006437488248779526384124594073909975123,
//0.006319504708542874639454201002308764146,
//0.006199276002235074196911046584546056692,
//0.006079992293847576688425871083154561347,
//0.0059570619206725005170821596323094127,
//0.005834819045240814336383827765075693605,
//0.005719388384369038930143958054941322189,
//0.005596675733755709578054471364794153487,
//0.005474782961579140183605574065950349905,
//0.00535440314116647237546731119550713629,
//0.005229788985673488113659068687866238179,
//0.00510803257835020299570949475764791714,
//0.004981422426134814174236709050092031248,
//0.004858348426887726737022354939199431101,
//0.004733858801461210408234592250664718449,
//0.004606298261030873988919331907254672842,
//0.004482391038819707497486977132439278648,
//0.004359004061052089255834385284060772392,
//0.004235512789095543552397860054270495311,
//0.004111358040218498523243884079647614271,
//0.00398785583312625657032013393177294347,
//0.003867157873719694223535459087770504993,
//0.003746823973938770164365807957551623986,
//0.003626361270307574755089596862944745226,
//0.003508437944578776408049991530901934311,
//0.003389679717524375027887240108270816563,
//0.003272952424774921813266370662631743471,
//0.003156409531300077061372055808874392824,
//0.003041783838773758508883249263021752995,
//0.039081789668528973957783989590097917244,
//};

//CURVATURE 141 TAP FIR FILTER UPDATED 7/6/2019
double firCoeff_1[] =
{
	-0.019298872721640179805913106747539131902,
	 0.001507626092377666471994190899863497179,
	 0.00148376024290682571350752372296710746,
	 0.001476576933963987805706330469490694668,
	 0.001494314580038068633258130368801630539,
	 0.001525062393416962624667632120178950572,
	 0.001578102749915626839063986786015902908,
	 0.001642038679406356439099967658989953634,
	 0.001729040519709702837872211667047395167,
	 0.001823736917492065241763743443925704923,
	 0.001936211477175667217132115638378309086,
	 0.00205946103318422734512749272539622325,
	 0.002197313839877959498053661135941183602,
	 0.002343920668001034900151946516189127578,
	 0.002505543347895702963740172108941806073,
	 0.002673821240127521869922588848567102104,
	 0.002856954483970750285209971863764621958,
	 0.003044313810009172897169671401229607,
	 0.003246976287341478244197201519227746758,
	 0.003452153476954638883916759439784982533,
	 0.003671495705716878044794215796287062403,
	 0.003892670289311545624849619784413334855,
	 0.004126763001811467948543032946417952189,
	 0.004362267264838450182840023927610673127,
	 0.004608626821505786355648304208898480283,
	 0.004855490729353644820120994296530625434,
	 0.005114500296164503634344100646558217704,
	 0.005368581861624015932887843405296735,
	 0.005637972586759239061737769560522792744,
	 0.005899074267995192688562866578649845906,
	 0.006179003925496437052422571412080287701,
	 0.006439418726004166211585033607889272389,
	 0.006735259156779774443457586841077500139,
	 0.007011534482100089706035905834369259537,
	 0.007255301038320626644262389959294523578,
	 0.007557543982211169068807787851937973755,
	 0.007838519330926139311310762991524825338,
	 0.008125675490521150345912815282645169646,
	 0.008400158718601618765275951261628506472,
	 0.008676873146511119086476959694209654117,
	 0.00894500838250711360899458668427541852,
	 0.009211256836036785938803816975450899918,
	 0.009479596044071440796852101584590855055,
	 0.009742375871564477873842946564764133655,
	 0.010000638214064175302020487379195401445,
	 0.010257868482826943204444525292728940258,
	 0.010507081229992257589977988629925675923,
	 0.01075324142813606151380945163964497624,
	 0.010990989116347093038084103966411930742,
	 0.01122426050861049032225658095285325544,
	 0.011448582507087154067071388396925613051,
	 0.011665670322749512391458104332286893623,
	 0.011874835580037223364824328086797322612,
	 0.012074550570127637674766596376230154419,
	 0.012265799697774701706998179417951178038,
	 0.012446758564447796269192281215509865433,
	 0.012618148187982581795441028305049258051,
	 0.012779521510314048246548246368092804914,
	 0.01292894036737647807389084420037761447,
	 0.013068680586425953857321324846907373285,
	 0.013196727995763770160952965682099602418,
	 0.013312599211112984345928289542371203424,
	 0.013417839634823940792029972612908750307,
	 0.013509823676979611922766011389285267796,
	 0.013594503085071383066684269635970849777,
	 0.013657372441482471328577652514013607288,
	 0.013723061544029460309679357976619940018,
	 0.013768447444528788409234820733217929956,
	 0.013793909008785402992014468281922745518,
	 0.01381408678107973707349653835763092502,
	 0.013818550882141364052890786240368470317,
	 0.01381408678107973707349653835763092502,
	 0.013793909008785402992014468281922745518,
	 0.013768447444528788409234820733217929956,
	 0.013723061544029460309679357976619940018,
	 0.013657372441482471328577652514013607288,
	 0.013594503085071383066684269635970849777,
	 0.013509823676979611922766011389285267796,
	 0.013417839634823940792029972612908750307,
	 0.013312599211112984345928289542371203424,
	 0.013196727995763770160952965682099602418,
	 0.013068680586425953857321324846907373285,
	 0.01292894036737647807389084420037761447,
	 0.012779521510314048246548246368092804914,
	 0.012618148187982581795441028305049258051,
	 0.012446758564447796269192281215509865433,
	 0.012265799697774701706998179417951178038,
	 0.012074550570127637674766596376230154419,
	 0.011874835580037223364824328086797322612,
	 0.011665670322749512391458104332286893623,
	 0.011448582507087154067071388396925613051,
	 0.01122426050861049032225658095285325544,
	 0.010990989116347093038084103966411930742,
	 0.01075324142813606151380945163964497624,
	 0.010507081229992257589977988629925675923,
	 0.010257868482826943204444525292728940258,
	 0.010000638214064175302020487379195401445,
	 0.009742375871564477873842946564764133655,
	 0.009479596044071440796852101584590855055,
	 0.009211256836036785938803816975450899918,
	 0.00894500838250711360899458668427541852,
	 0.008676873146511119086476959694209654117,
	 0.008400158718601618765275951261628506472,
	 0.008125675490521150345912815282645169646,
	 0.007838519330926139311310762991524825338,
	 0.007557543982211169068807787851937973755,
	 0.007255301038320626644262389959294523578,
	 0.007011534482100089706035905834369259537,
	 0.006735259156779774443457586841077500139,
	 0.006439418726004166211585033607889272389,
	 0.006179003925496437052422571412080287701,
	 0.005899074267995192688562866578649845906,
	 0.005637972586759239061737769560522792744,
	 0.005368581861624015932887843405296735,
	 0.005114500296164503634344100646558217704,
	 0.004855490729353644820120994296530625434,
	 0.004608626821505786355648304208898480283,
	 0.004362267264838450182840023927610673127,
	 0.004126763001811467948543032946417952189,
	 0.003892670289311545624849619784413334855,
	 0.003671495705716878044794215796287062403,
	 0.003452153476954638883916759439784982533,
	 0.003246976287341478244197201519227746758,
	 0.003044313810009172897169671401229607,
	 0.002856954483970750285209971863764621958,
	 0.002673821240127521869922588848567102104,
	 0.002505543347895702963740172108941806073,
	 0.002343920668001034900151946516189127578,
	 0.002197313839877959498053661135941183602,
	 0.00205946103318422734512749272539622325,
	 0.001936211477175667217132115638378309086,
	 0.001823736917492065241763743443925704923,
	 0.001729040519709702837872211667047395167,
	 0.001642038679406356439099967658989953634,
	 0.001578102749915626839063986786015902908,
	 0.001525062393416962624667632120178950572,
	 0.001494314580038068633258130368801630539,
	 0.001476576933963987805706330469490694668,
	 0.00148376024290682571350752372296710746,
	 0.001507626092377666471994190899863497179,
	-0.019298872721640179805913106747539131902
};

//HORIZONTAL 35 TAP ALIGNMENT FIR FILTER UPDATED 7/6/2019
double alignmentFirCoeff_1[] =
{
	 0.014016608360150150866529905613333539804,
	-0.05731375029742723919978786284445959609,
	-0.015773595421299003993542697799057350494,
	 0.000552949604053197389143259332655588878,
	 0.013281646385411951735711078015356179094,
	 0.022609481326999473355110481520569010172,
	 0.023960939436268480218217291621840558946,
	 0.014203136081761035328097086960497108521,
	-0.005246590937802340752493801545597307268,
	-0.027736302943654998764966279622967704199,
	-0.043215331573620015259695748000012827106,
	-0.04146684543702819358834688046044902876,
	-0.016260730393295997481262205042185087223,
	 0.031645443642568132913694967101037036628,
	 0.093559700369633944094793776002916274592,
	 0.155098517738574803725981610114104114473,
	 0.200321051871077598915960038539196830243,
	 0.216952551989159886369762375579739455134,
	 0.200321051871077598915960038539196830243,
	 0.155098517738574803725981610114104114473,
	 0.093559700369633944094793776002916274592,
	 0.031645443642568132913694967101037036628,
	-0.016260730393295997481262205042185087223,
	-0.04146684543702819358834688046044902876,
	-0.043215331573620015259695748000012827106,
	-0.027736302943654998764966279622967704199,
	-0.005246590937802340752493801545597307268,
	 0.014203136081761035328097086960497108521,
	 0.023960939436268480218217291621840558946,
	 0.022609481326999473355110481520569010172,
	 0.013281646385411951735711078015356179094,
	 0.000552949604053197389143259332655588878,
	-0.015773595421299003993542697799057350494,
	-0.05731375029742723919978786284445959609,
	 0.014016608360150150866529905613333539804
};

CoreGeomCalculationPacket rawCalc_256Buffer[512];
std::mutex rawCalc_256Mutex;
std::condition_variable condVar_rawCalc256;
int rawCalc_256Cnt;

std::queue<CoreGeomCalculationPacket> geoCalculation_outputBuffer;
std::mutex geoCalcuation_outputMutex;
std::condition_variable condVar_geoCalcOutput;

std::queue<WTGS_Exception> exceptionOutQue;
std::mutex exceptionOutMutex;
std::condition_variable exceptionOutCondVar;

bool calculationProcessorEnable = false;

ExceptionAnalysis excAnalysis;

//CONSTRUCTORS
GeoCalculations::GeoCalculations()
{
	excAnalysis = ExceptionAnalysis();
	rawCalc_256Cnt = 0;
	calculationProcessorEnable = true;
	memset(rawCalc_256Buffer, 0, sizeof(rawCalc_256Buffer));
	int buffSize = geoCalculation_outputBuffer.size();
	for (int i = 0; i < buffSize; i++)
	{
		geoCalculation_outputBuffer.pop();
	}

	int excBuffSize = exceptionOutQue.size();
	for (int i = 0; i < excBuffSize; i++)
	{
		exceptionOutQue.pop();
	}
}

GeoCalculations::~GeoCalculations()
{

}

//LOOP FOR PERFORMING ALL THE CALCULATIONS ON BUFFER
bool GeoCalculations::calculationProcessor()
{
	//BUFFER FOR HOLDING EXCPETIONS -- USED FOR CHAINING EXCEPTIONS TOGETHER
	WTGS_Exception exceptionBuffer0[100];
	int exceptionBuffer0Count= 0;

	while (calculationProcessorEnable)
	{
		std::unique_lock<std::mutex> l(rawCalc_256Mutex);
		condVar_rawCalc256.wait_for(l, std::chrono::milliseconds(8));
		if (!calculationProcessorEnable)
		{
			return true;
		}
		//FINSH THE CALCULATION PACKETS BY DOING CALCULATION//

		////SHORT CALCULATIONS FOR THE FIRST 70 VALUES
		if (rawCalc_256Cnt < 510)
		{
			for (int i = rawCalc_256Cnt - 1; i > rawCalc_256Cnt - 71 - 1 && i >= 0; i--)
			{
				if (rawCalc_256Buffer[i].secondCalcsComplete == false)
				{
					rawCalc_256Buffer[i].R_HORIZ_MCO_31_NOC = 0;
					rawCalc_256Buffer[i].R_HORIZ_MCO_62_NOC = 0;
					rawCalc_256Buffer[i].R_HORIZ_MCO_124_NOC = 0;
					rawCalc_256Buffer[i].filteredCurvature = 0;
					rawCalc_256Buffer[i].fraWarp62_BaseLength = 0;
					rawCalc_256Buffer[i].twist11 = 0;
					rawCalc_256Buffer[i].verticalBounce = 0;
					rawCalc_256Buffer[i].combinationLeft = 0;
					rawCalc_256Buffer[i].combinationRight = 0;
					rawCalc_256Buffer[i].multipleTwist11 = 0;
					rawCalc_256Buffer[i].fraRunoff = 0;
					rawCalc_256Buffer[i].fraWarp62 = 0;
					rawCalc_256Buffer[i].variationCurvature = 0;
					rawCalc_256Buffer[i].relAlignmentLeft31_NOC = 0;
					rawCalc_256Buffer[i].relAlignmentLeft31_OC = 0;
					rawCalc_256Buffer[i].relAlignmentLeft62_NOC = 0;
					rawCalc_256Buffer[i].relAlignmentLeft62_OC = 0;
					rawCalc_256Buffer[i].relAlignmentRight31_NOC = 0;
					rawCalc_256Buffer[i].relAlignmentRight31_OC = 0;
					rawCalc_256Buffer[i].relAlignmentRight62_NOC = 0;
					rawCalc_256Buffer[i].relAlignmentRight62_OC = 0;
					rawCalc_256Buffer[i].fraHarmonic = 0;
					
					for (int j = 0; j < 43; j++)
					{
						rawCalc_256Buffer[i].variationCrosslevel[j] = 0;
					}

					//DO TH ESHORT VERSION OF EXCEPTION ANALYSIS

					//SET THE SECOND CALCS COMPLETE FLAG TO TRUE
					rawCalc_256Buffer[i].secondCalcsComplete = true;

					//PLACE THIS PACKET INTO THE OUTPUT BUFFER -- THIS ALSO NOTIFIES THE CONDITIONAL VARIABLE
					putCompleteCalcPacket_outputBuffer(rawCalc_256Buffer[i]);

					//PERFORM EXCEPTION ANALYSIS
					WTGS_Exception foundExceptionArray[100];
					int foundExceptionsCount = 0;
					WTGS_Exception completeExceptionArray[100];
					int completeExceptionArrayCount = 0;

					//FIND EXCEPTIONS
					excAnalysis.PerformExceptionAnalysis(rawCalc_256Buffer[i], true, foundExceptionArray, foundExceptionsCount);

					//CHAIN EXCEPTIONS TOGETHER AND RETURN THE 'COMPLETE' ONES
					chainExceptionsAndShift(foundExceptionArray, exceptionBuffer0, foundExceptionsCount, exceptionBuffer0Count, completeExceptionArray, completeExceptionArrayCount);
					
					//PLACE THE COMPLETE EXCEPTIONS INTO AN OUTPUT QUE -- INERTIALSYSTEM.CPP WILL RETRIEVE
					for (int o = 0; o < completeExceptionArrayCount; o++)
					{
						putExceptionOutputQue(completeExceptionArray[o]);
					}

				}
			}
		}



		//CURVATURE FIR FILTER IS 141 PACKETS IN LENGTH....SO MAKE SOME BOUNDARIES
		for(int i = rawCalc_256Cnt - 102; i >= 70; i--)
		{
			if (rawCalc_256Buffer[i].secondCalcsComplete == false)
			{
				//DO THE CALCUATIONS

				//## 140 FOOT CURVATURE FILTER ##//
				double rawCurvVals[141];
				int arrayIndex = 0;
				for (int j = i - 70; j <= i + 70; j++)
				{
					rawCurvVals[arrayIndex] = rawCalc_256Buffer[j].rawCurvature;
					arrayIndex++;
				}
				rawCalc_256Buffer[i].filteredCurvature = firFilter(0, rawCurvVals, 141);

				//## 34 FOOT HORIZONTAL ALIGNMENT FILTER ##//
				double rawNOC_AlignmentVals[35];
				int rawNocAlignIndex = 0;
				for (int j = i - 17; j <= i + 17; j++)
				{
					rawNOC_AlignmentVals[rawNocAlignIndex] = rawCalc_256Buffer[j].L_HORIZ_MCO_62_NOC;
					rawNocAlignIndex++;
				}
				rawCalc_256Buffer[i].R_HORIZ_MCO_62_NOC = firFilter(1, rawNOC_AlignmentVals, 35);

				//## CALCULATE 11 FOOT TWIST ##//
				rawCalc_256Buffer[i].twist11 = rawCalc_256Buffer[i - 5].crosslevel_OC - rawCalc_256Buffer[i + 6].crosslevel_OC;
				//SWA
				rawCalc_256Buffer[i].twist11_SWA = rawCalc_256Buffer[i - 5].crosslevel_OC_SWA - rawCalc_256Buffer[i + 6].crosslevel_OC_SWA;

				//## CALCULATE 31 FOOT TWIST ##//
				rawCalc_256Buffer[i].twist31 = rawCalc_256Buffer[i - 15].crosslevel_OC - rawCalc_256Buffer[i + 16].crosslevel_OC;

				//## CALCULATE VERTICAL BOUNCE ##//
				double maxLeftProfile11 = -999.0;
				double maxRightProfile11 = -999.0;
				for (int j = i - 4; j <= i + 5; j++)
				{
					double tmp_L_VMCO11 = 0;
					double tmp_R_VMCO11 = 0;
					tmp_L_VMCO11 = rawCalc_256Buffer[j].L_VERT_MCO_11_OC;
					tmp_R_VMCO11 = rawCalc_256Buffer[j].R_VERT_MCO_11_OC;

					if (tmp_L_VMCO11 < 0.0)
					{
						tmp_L_VMCO11 = -1.0 * tmp_L_VMCO11;
					}
					if (tmp_R_VMCO11 < 0.0)
					{
						tmp_R_VMCO11 = -1.0 * tmp_R_VMCO11;
					}

					if (maxLeftProfile11 < tmp_L_VMCO11)
					{
						maxLeftProfile11 = tmp_L_VMCO11;
					}
					if (maxRightProfile11 < tmp_R_VMCO11)
					{
						maxRightProfile11 = tmp_R_VMCO11;
					}
				}
			
				if (maxLeftProfile11 > maxRightProfile11)
				{
					rawCalc_256Buffer[i].verticalBounce = maxRightProfile11;
				}
				else
				{
					rawCalc_256Buffer[i].verticalBounce = maxLeftProfile11;
				}

				//COMBINATION SURFACE
				//RE-USING THE MAX LEFT & RIGHT PROIFLE 11 VALUES CALCULATED ABOVE IN VERTICAL BOUNCE
				//DETERMINE THE MAX TWIST CALCULATION OVER THE +/- 10 FOOT RANGE
				double maxTwist11 = -999.0;
				for (int j = -4; j <= 5; j++)
				{
					double tmpTwist = 0.0;
					tmpTwist = rawCalc_256Buffer[i - 5 + j].crosslevel_OC - rawCalc_256Buffer[i + 6 + j].crosslevel_OC;

					if (tmpTwist < 0.0)
					{
						tmpTwist = -1.0 * tmpTwist;
					}
					if (maxTwist11 < tmpTwist)
					{
						maxTwist11 = tmpTwist;
					}
				}

				//## CALCULATE LEFT COMBINATION ##//
				if (maxTwist11 < maxLeftProfile11)
				{
					rawCalc_256Buffer[i].combinationLeft = maxTwist11;
				}
				else
				{
					rawCalc_256Buffer[i].combinationLeft = maxLeftProfile11;
				}

				//## ASSIGN RIGHT COMBINATION ##//
				if (maxTwist11 < maxRightProfile11)
				{
					rawCalc_256Buffer[i].combinationRight = maxTwist11;
				}
				else
				{
					rawCalc_256Buffer[i].combinationRight = maxRightProfile11;
				}

				//////////////MULTIPLE TWIST -- AVG OF LAST 7 PEAK-TO-PEAK 11 FOOT TWIST 'CYCLES'

				////////////bool lastTwistNeg = false;
				////////////double twistOnSignChange[7];
				////////////int twistOnSignChangeIndex = 0;
				////////////double currentNegPeak = 0;
				////////////double currentPosPeak = 0;
				////////////int peaks_cnt = 0;
				////////////double lotOPeaks[256];
				////////////for (int j = 60; j >= -60; j--)
				//////////////for(int j = -60; j <= 60; j++)
				////////////{
				////////////	//CALCULATE 11 FOOT TWIST FOR CURRENT LOCATION
				////////////	double tmpTwist11 = rawCalc_256Buffer[i + j - 5].crosslevel_NOC - rawCalc_256Buffer[i + j + 6].crosslevel_NOC;
				////////////	//FOR FIRST VALUE , SETUP AS STARTING WITH +/- TWIST VALUE
				////////////	if (j == 60)
				////////////	{
				////////////		if (tmpTwist11 < 0.0)
				////////////			lastTwistNeg = true;
				////////////		else
				////////////			lastTwistNeg = false;
				////////////	}

				////////////	if (lastTwistNeg && tmpTwist11 >= 0.0)
				////////////	{
				////////////		//CHANGE TO POSITIVE SIDE
				////////////		//CALCULATE THE MAX PEAK-PEAK OF TWIST VALUES
				////////////		twistOnSignChange[twistOnSignChangeIndex] = currentPosPeak - currentNegPeak;
				////////////		if (twistOnSignChangeIndex < 6)
				////////////		{
				////////////			twistOnSignChangeIndex++;
				////////////		}
				////////////		else
				////////////		{
				////////////			twistOnSignChangeIndex = 0;
				////////////		}
				////////////		lotOPeaks[peaks_cnt] = currentPosPeak - currentNegPeak;
				////////////		peaks_cnt++;
				////////////		currentPosPeak = tmpTwist11;
				////////////		lastTwistNeg = false;
				////////////	}
				////////////	//CHANGE TO NEGATIVE SIDE
				////////////	//CALCULATE THE MAX PEAK-PEAK OF TWIST VALUES
				////////////	if (!lastTwistNeg && tmpTwist11 < 0.0)
				////////////	{
				////////////		twistOnSignChange[twistOnSignChangeIndex] = currentPosPeak - currentNegPeak;
				////////////		if (twistOnSignChangeIndex < 6)
				////////////		{
				////////////			twistOnSignChangeIndex++;
				////////////		}
				////////////		else
				////////////		{
				////////////			twistOnSignChangeIndex = 0;
				////////////		}
				////////////		twistOnSignChangeIndex++;
				////////////		lotOPeaks[peaks_cnt] = currentPosPeak - currentNegPeak;
				////////////		peaks_cnt++;
				////////////		currentNegPeak = tmpTwist11;
				////////////		lastTwistNeg = true;
				////////////	}

				////////////	//NO SIGN CHANGE -- UPDATE THE MAX / MIN VALUES
				////////////	if (lastTwistNeg)
				////////////	{
				////////////		//CHECK FOR A 'LARGER' NEGATIVE NUMBER
				////////////		if (tmpTwist11 < currentNegPeak)
				////////////		{
				////////////			currentNegPeak = tmpTwist11;
				////////////		}
				////////////	}
				////////////	else
				////////////	{
				////////////		//CHECK FOR A 'LARGE' POSITIVE NUMBER
				////////////		if (tmpTwist11 > currentPosPeak)
				////////////		{
				////////////			currentPosPeak = tmpTwist11;
				////////////		}
				////////////	}

				////////////}
				////////////double ppSum = 0.0;
				////////////double ppCnt = 0.0;
				////////////for (int j = 0; j < 7 && j < peaks_cnt; j++)
				////////////{
				////////////	ppSum = ppSum + twistOnSignChange[j];
				////////////	ppCnt = ppCnt + 1.0;
				////////////}
				//////////////for (int j = 0; j < peaks_cnt; j++)
				//////////////{
				//////////////	ppSum = ppSum + lotOPeaks[j];
				//////////////	ppCnt = ppCnt + 1.0;
				//////////////}
				////////////rawCalc_256Buffer[i].multipleTwist11 = ppSum / ppCnt;


				//## CALCULATE FRA 31 FOOT RUNOFF ##//
				double tmp31Runoff = -999.0;
				double avg31Profile1 = 0.0;
				double avg31Profile2 = 0.0;
				avg31Profile1 = (rawCalc_256Buffer[i - 15].L_VERT_MCO_31_OC + rawCalc_256Buffer[i - 15].R_VERT_MCO_31_OC) / 2.0;
				avg31Profile2 = (rawCalc_256Buffer[i + 16].L_VERT_MCO_31_OC + rawCalc_256Buffer[i + 16].R_VERT_MCO_31_OC) / 2.0;
				rawCalc_256Buffer[i].fraRunoff = avg31Profile1 - avg31Profile2;

				//## FRA WARP62 ##//
				double minXL = 999.0;
				double minXL_SWA = 999.0;
				double maxXL = -999.0;
				double maxXL_SWA = -999.0;
				double maxIndex = 0.0;
				double maxIndex_SWA = 0.0;
				double minIndex = 0.0;
				double minIndex_SWA = 0.0;
				for (int j = i - 30; j <= i + 31; j++)
				{
					if (minXL > rawCalc_256Buffer[j].crosslevel_OC)
					{
						minXL = rawCalc_256Buffer[j].crosslevel_OC;
						minIndex = j - i + 31;
					}
					if (maxXL < rawCalc_256Buffer[j].crosslevel_OC)
					{
						maxXL = rawCalc_256Buffer[j].crosslevel_OC;
						maxIndex = j - i + 31;
					}
					//SWA
					if (minXL_SWA > rawCalc_256Buffer[j].crosslevel_OC_SWA)
					{
						minXL_SWA = rawCalc_256Buffer[j].crosslevel_OC_SWA;
						minIndex_SWA = j - i + 31;
					}
					if (maxXL_SWA < rawCalc_256Buffer[j].crosslevel_OC_SWA)
					{
						maxXL_SWA = rawCalc_256Buffer[j].crosslevel_OC_SWA;
						maxIndex_SWA = j - i + 31;
					}
				}
				rawCalc_256Buffer[i].fraWarp62 = maxXL - minXL;
				rawCalc_256Buffer[i].fraWarp62_BaseLength = maxIndex - minIndex;
				rawCalc_256Buffer[i].fraWarp62_SWA = maxXL_SWA - minXL_SWA;


				//VARIATION IN CROSS LEVEL
				//CALCULATE 'TWIST' FOR BASE LENGTHS 20 - 62
				int vxlToLeft = 10;
				int vxlToRight = 10;
				double vXLValues[43];
				double vXLValues_SWA[43];
				for (int j = 20; j <= 62; j++)
				{
					if (j % 2 == 0)
					{
						vxlToLeft++;
					}
					else
					{
						vxlToRight++;
					}
					vXLValues[j - 20] = rawCalc_256Buffer[i - vxlToLeft].crosslevel_OC - rawCalc_256Buffer[i + vxlToRight].crosslevel_OC;
					vXLValues_SWA[j - 20] = rawCalc_256Buffer[i - vxlToLeft].crosslevel_OC_SWA - rawCalc_256Buffer[i + vxlToRight].crosslevel_OC_SWA;
				}
				memcpy(rawCalc_256Buffer[i].variationCrosslevel, vXLValues, sizeof(vXLValues));
				memcpy(rawCalc_256Buffer[i].variationCrosslevel_SWA, vXLValues_SWA, sizeof(vXLValues_SWA));

				//## CALCULATE VARIATION IN CURVATURE ##//
				//CALCULATE 'MAXIMUM TWIST' OVER 62 FEET OF FILTERED CURVATURE
				double minCurv = 999.0;
				double maxCurv = -999.0;
				double maxCurvIndex = 0.0;
				double minCurvIndex = 0.0;
				//NOT CENTERED AROUND THE i POINT, BECAUSE CURVATURE FILTER HASN'T BEEN APPLIED TO i-X ELEMENT YET
				for(int j = i; j < i + 62; j++)
				{
					if (minCurv > rawCalc_256Buffer[j].filteredCurvature)
					{
						minCurv = rawCalc_256Buffer[j].filteredCurvature;
						minCurvIndex = (double)(j);
					}
					if (maxCurv < rawCalc_256Buffer[j].filteredCurvature)
					{
						maxCurv = rawCalc_256Buffer[j].filteredCurvature;
						maxCurvIndex = (double)(j);
					}
				}

				if (rawCalc_256Cnt < 510)
				{
					rawCalc_256Buffer[i].variationCurvature = 0.0;
					rawCalc_256Buffer[i].variationCurvature_BaseLength = 0.0;
				}
				else
				{
					rawCalc_256Buffer[i].variationCurvature = maxCurv - minCurv;
					rawCalc_256Buffer[i].variationCurvature_BaseLength = maxCurvIndex - minCurvIndex;
				}

				//## FRA ALIGNMENT 62 FOOT - RELATIVE ##//
				double relR_NOC_sum62 = 0.0;
				double relR_NOC_cnt62 = 0.0;
				double relR_OC_sum62 = 0.0;
				double relR_OC_cnt62 = 0.0;
				double relL_NOC_sum62 = 0.0;
				double relL_NOC_cnt62 = 0.0;
				double relL_OC_sum62 = 0.0;
				double relL_OC_cnt62 = 0.0;

				//NOT CENTERED AROUND THE i POINT, BECAUSE CURVATURE FILTER HASN'T BEEN APPLIED TO i-X ELEMENT YET
				//Actually does 100 ft
				for (int j = i; j < i + 100; j++)
				{
					relR_NOC_cnt62 += 1.0;
					relR_OC_cnt62 += 1.0;
					relL_NOC_cnt62 += 1.0;
					relL_OC_cnt62 += 1.0;
					
					relR_NOC_sum62 += rawCalc_256Buffer[j].R_HORIZ_MCO_62_NOC;
					relR_OC_sum62 += rawCalc_256Buffer[j].R_HORIZ_MCO_62_OC;
					relL_NOC_sum62 += rawCalc_256Buffer[j].L_HORIZ_MCO_62_NOC;
					relL_OC_sum62 += rawCalc_256Buffer[j].L_HORIZ_MCO_62_OC;
				}

				if (rawCalc_256Cnt < 510)
				{
					rawCalc_256Buffer[i].relAlignmentLeft62_NOC = 0.0;
					rawCalc_256Buffer[i].relAlignmentLeft62_OC = 0.0;
					rawCalc_256Buffer[i].relAlignmentRight62_NOC = 0.0;
					rawCalc_256Buffer[i].relAlignmentRight62_OC = 0.0;
				}
				else
				{
					rawCalc_256Buffer[i].relAlignmentLeft62_NOC = rawCalc_256Buffer[i + 50].L_HORIZ_MCO_62_NOC - (relL_NOC_sum62 / relL_NOC_cnt62);
					rawCalc_256Buffer[i].relAlignmentLeft62_OC = rawCalc_256Buffer[i + 50].L_HORIZ_MCO_62_OC - (relL_OC_sum62 / relL_OC_cnt62);
					rawCalc_256Buffer[i].relAlignmentRight62_NOC = rawCalc_256Buffer[i + 50].R_HORIZ_MCO_62_NOC - (relR_NOC_sum62 / relR_NOC_cnt62);
					rawCalc_256Buffer[i].relAlignmentRight62_OC = rawCalc_256Buffer[i + 50].R_HORIZ_MCO_62_OC - (relR_OC_sum62 / relR_OC_cnt62);
				}


				//## FRA ALIGNMENT 31 FOOT - RELATIVE ##//
				double relR_NOC_sum31 = 0.0;
				double relR_NOC_cnt31 = 0.0;
				double relR_OC_sum31 = 0.0;
				double relR_OC_cnt31 = 0.0;
				double relL_NOC_sum31 = 0.0;
				double relL_NOC_cnt31 = 0.0;
				double relL_OC_sum31 = 0.0;
				double relL_OC_cnt31 = 0.0;

				//NOT CENTERED AROUND THE i POINT, BECAUSE CURVATURE FILTER HASN'T BEEN APPLIED TO i-X ELEMENT YET
				for(int j = i; j < i + 31; j++)
				{
					relR_NOC_cnt31 += 1.0;
					relR_OC_cnt31 += 1.0;
					relL_NOC_cnt31 += 1.0;
					relL_OC_cnt31 += 1.0;

					relR_NOC_sum31 += rawCalc_256Buffer[j].R_HORIZ_MCO_31_NOC;
					relR_OC_sum31 += rawCalc_256Buffer[j].R_HORIZ_MCO_31_OC;
					relL_NOC_sum31 += rawCalc_256Buffer[j].L_HORIZ_MCO_31_NOC;
					relL_OC_sum31 += rawCalc_256Buffer[j].L_HORIZ_MCO_31_OC;
				}


				if (rawCalc_256Cnt < 510)
				{
					rawCalc_256Buffer[i].relAlignmentLeft31_NOC = 0.0;
					rawCalc_256Buffer[i].relAlignmentLeft31_OC = 0.0;
					rawCalc_256Buffer[i].relAlignmentRight31_NOC = 0.0;
					rawCalc_256Buffer[i].relAlignmentRight31_OC = 0.0;
				}
				else
				{
					rawCalc_256Buffer[i].relAlignmentLeft31_NOC = rawCalc_256Buffer[i + 15].L_HORIZ_MCO_31_NOC - (relL_NOC_sum31 / relL_NOC_cnt31);
					rawCalc_256Buffer[i].relAlignmentLeft31_OC = rawCalc_256Buffer[i + 15].L_HORIZ_MCO_31_OC - (relL_OC_sum31 / relL_OC_cnt31);
					rawCalc_256Buffer[i].relAlignmentRight31_NOC = rawCalc_256Buffer[i + 15].R_HORIZ_MCO_31_NOC - (relR_NOC_sum31 / relR_NOC_cnt31);
					rawCalc_256Buffer[i].relAlignmentRight31_OC = rawCalc_256Buffer[i + 15].R_HORIZ_MCO_31_OC - (relR_OC_sum31 / relR_OC_cnt31);
				}


				//## CALCULATE GAGE VARIATION ##//
				//CALCULATE 'MAXIMUM TWIST' OVER 10 FEET -- IGNORING GAGE UNDER 56.5"
				double minGage20 = 999.0;
				double maxGage20 = -999.0;
				for (int j = i - 4; j <= i + 5; j++)
				{
					if (minGage20 > rawCalc_256Buffer[j].fullGage_filtered && rawCalc_256Buffer[j].fullGage_filtered >= 56.5 && rawCalc_256Buffer[j].fullGage_filtered < 58.5)
					{
						minGage20 = rawCalc_256Buffer[j].fullGage_filtered;
					}
					if (maxGage20 < rawCalc_256Buffer[j].fullGage_filtered && rawCalc_256Buffer[j].fullGage_filtered >= 56.5 && rawCalc_256Buffer[j].fullGage_filtered < 58.5)
					{
						maxGage20 = rawCalc_256Buffer[j].fullGage_filtered;
					}
				}

				if (maxGage20 != -999.0 && minGage20 != 999.0)
				{
					rawCalc_256Buffer[i].variationGage = maxGage20 - minGage20;
				}
				else
				{
					rawCalc_256Buffer[i].variationGage = 0.0;
				}

				//## CALCULATE FRA HARMONIC ROCK ##//
				//LOOKS OVER A 117 FOOT WINDOW (THIS ENSURES THAT AT LEAST 3 JOINTS ON ONE SIDE OF THE TRACK, SPACED 39' ARE PRESENT...
				//WE NEED 6 JOINTS; 3 LEFT AND 3 RIGHT
				//WE ARE GOING TO FIND THE MIN x3 AND MAX x3 XL VALUES, EACH SPACED AT LEAST 30FT A PART (BETWEEN EACH MAX / BETWEEN EACH MIN)
				rawCalc_256Buffer[i].fraHarmonic = 0.0;
				double hrMin1 = 999.0;
				double hrMin2 = 999.0;
				double hrMin3 = 999.0;
				//double hrMin4 = 999.0;
				double hrMax1 = -999.0;
				double hrMax2 = -999.0;
				double hrMax3 = -999.0;
				//double hrMax4 = -999.0;
				int hrMaxIndex1 = 0;
				int hrMaxIndex2 = 0;
				int hrMaxIndex3 = 0;
				int hrMinIndex1 = 0;
				int hrMinIndex2 = 0;
				int hrMinIndex3 = 0;
				int hrCounter = 0;
				//START 58 FEET BACK AND MOVE TOWARDS 59 FEET IN FRONT
				for (int o = i - 58; o <= i + 59; o++)
				{
					if (hrCounter < 39)
					{
						//COMPARE MAX
						if (rawCalc_256Buffer[o].crosslevel_OC > hrMax1)
						{
							hrMax1 = rawCalc_256Buffer[o].crosslevel_OC;
							hrMaxIndex1 = hrCounter;
						}
						//COMPARE MIN
						if (rawCalc_256Buffer[o].crosslevel_OC < hrMin1)
						{
							hrMin1 = rawCalc_256Buffer[o].crosslevel_OC;
							hrMinIndex1 = hrCounter;
						}
					}
					else
					{
						if (hrCounter < 78)
						{
							//COMPARE MAX
							if (rawCalc_256Buffer[o].crosslevel_OC > hrMax2)
							{
								hrMax2 = rawCalc_256Buffer[o].crosslevel_OC;
								hrMaxIndex2 = hrCounter;
							}
							//COMPARE MIN
							if (rawCalc_256Buffer[o].crosslevel_OC < hrMin2)
							{
								hrMin2 = rawCalc_256Buffer[o].crosslevel_OC;
								hrMinIndex2 = hrCounter;
							}
						}
						else
						{
							//COMPARE MAX
							if (rawCalc_256Buffer[o].crosslevel_OC > hrMax3)
							{
								hrMax3 = rawCalc_256Buffer[o].crosslevel_OC;
								hrMaxIndex3 = hrCounter;
							}
							//COMPARE MIN
							if (rawCalc_256Buffer[o].crosslevel_OC < hrMin3)
							{
								hrMin3 = rawCalc_256Buffer[o].crosslevel_OC;
								hrMinIndex3 = hrCounter;
							}
						}
					}
					hrCounter++;
				}

				//NEED TO MAKE SURE THAT THE JOINT STAGGERING IS AT LEAST > 10 FEET, OR IT DOESN"T MATTER
				//COMPARE THE LEGS OF EACH JOINT --- LEG = LEFT TO RIGHT, RIGHT TO LEFT
				double leg1 = std::abs(hrMax1 - hrMin1);
				double leg2 = std::abs(hrMax2 - hrMin1);
				double leg3 = std::abs(hrMax2 - hrMin2);
				double leg4 = std::abs(hrMax3 - hrMin2);
				double leg5 = std::abs(hrMax3 - hrMin3);
				if (leg1 > 1.25 && leg2 > 1.25 && leg3 > 1.25 && leg4 > 1.25 && leg5 > 1.25)
				{
					//MAKE SURE THAT THE INDEX ARE ALL > 10 feet apart
					int dLeg1 = std::abs(hrMaxIndex1 - hrMinIndex1);
					int dLeg2 = std::abs(hrMaxIndex2 - hrMinIndex1);
					int dLeg3 = std::abs(hrMaxIndex2 - hrMinIndex2);
					int dLeg4 = std::abs(hrMaxIndex3 - hrMinIndex2);
					int dLeg5 = std::abs(hrMaxIndex3 - hrMinIndex3);
					if (leg1 >= 10 && leg2 >= 10 && leg3 >= 10 && leg4 >= 10 && leg5 >= 10)
					{
						//DETERMINE THE MAX LEG XL
						double maxLegVal = leg1;
						if (leg2 > maxLegVal)
						{
							maxLegVal = leg2;
						}
						if (leg3 > maxLegVal)
						{
							maxLegVal = leg3;
						}
						if (leg4 > maxLegVal)
						{
							maxLegVal = leg4;
						}
						if (leg5 > maxLegVal)
						{
							maxLegVal = leg5;
						}
						rawCalc_256Buffer[i].fraHarmonic = maxLegVal;
					}
				}
				//END OF HARMONIC CALCULATION

				

				//SET THE SECOND CALCS COMPLETE FLAG TO TRUE
				rawCalc_256Buffer[i].secondCalcsComplete = true;

				//PLACE THIS PACKET INTO THE OUTPUT BUFFER -- THIS ALSO NOTIFIES THE CONDITIONAL VARIABLE
				putCompleteCalcPacket_outputBuffer(rawCalc_256Buffer[i]);

				//PERFORM EXCEPTION ANALYSIS
				WTGS_Exception foundExceptionArray[100];
				int foundExceptionsCount = 0;
				WTGS_Exception completeExceptionArray[100];
				int completeExceptionArrayCount = 0;

				//FIND EXCEPTIONS
				excAnalysis.PerformExceptionAnalysis(rawCalc_256Buffer[i], false, foundExceptionArray, foundExceptionsCount);

				//CHAIN EXCEPTIONS TOGETHER AND RETURN THE 'COMPLETE' ONES
				chainExceptionsAndShift(foundExceptionArray, exceptionBuffer0, foundExceptionsCount, exceptionBuffer0Count, completeExceptionArray, completeExceptionArrayCount);

				//PLACE THE COMPLETE EXCEPTIONS INTO AN OUTPUT QUE -- INERTIALSYSTEM.CPP WILL RETRIEVE
				for (int o = 0; o < completeExceptionArrayCount; o++)
				{
					if (completeExceptionArray[o].exceptionType == XL_REV_25FT)
					{
						if (completeExceptionArray[o].exceptionLength >= 25)
						{
							putExceptionOutputQue(completeExceptionArray[o]);
						}
					}
					else
					{
						if (completeExceptionArray[o].exceptionType == FRA_VARIATION0_XL || completeExceptionArray[o].exceptionType == GAGE_TIGHT)
						{
							if (completeExceptionArray[o].exceptionLength >= 10)
							{
								putExceptionOutputQue(completeExceptionArray[o]);
							}
						}
						else if (completeExceptionArray[o].exceptionType == FRA_RUNOFF)
						{
							if (completeExceptionArray[o].exceptionLength >= 5)
							{
								putExceptionOutputQue(completeExceptionArray[o]);
							}
						}
						else
						{
							putExceptionOutputQue(completeExceptionArray[o]);
						}
					}
				}
			}
		}
	}
	return true;
}

//PLACE AN INCOMING CORE GEOM CALC PACKET INTO 256 BUFFER
bool GeoCalculations::putRawCalcPacket_512Buffer(CoreGeomCalculationPacket p)
{
	//POSITION 0 IS THE NEWEST DATA
	//SHIFT BUFFER BY 1 FOOT -- THROWING LAST POSITION OUT
	rawCalc_256Mutex.lock();
	memmove(&rawCalc_256Buffer[1], &rawCalc_256Buffer[0], (sizeof CoreGeomCalculationPacket()) * 511);

	//WRITE IN THE NEW DATA
	rawCalc_256Buffer[0] = p;

	if (rawCalc_256Cnt < 512)
	{
		rawCalc_256Cnt++;
	}
	rawCalc_256Mutex.unlock();

	condVar_rawCalc256.notify_one();

	return true;
}

//PLACE AN OUTGOING CORE GEOM CALC PACKET INTO QUE
bool putCompleteCalcPacket_outputBuffer(CoreGeomCalculationPacket p)
{
	std::unique_lock<std::mutex> l(geoCalcuation_outputMutex);
	
	geoCalculation_outputBuffer.push(p);

	l.unlock();
	condVar_geoCalcOutput.notify_one();

	return true;
}

//GET AN OUTGOING CORE GEOM CALC PACKET FROM QUE
bool GeoCalculations::getCompleteCalcPacket_outputBuffer(CoreGeomCalculationPacket &p)
{
	std::unique_lock<std::mutex> l(geoCalcuation_outputMutex);
	condVar_geoCalcOutput.wait_for(l, std::chrono::milliseconds(100), [] {return geoCalculation_outputBuffer.size() > 0; });
	int bufferSize = geoCalculation_outputBuffer.size();
	if (bufferSize > 0)
	{
		p = geoCalculation_outputBuffer.front();
		geoCalculation_outputBuffer.pop();
		l.unlock();
		return true;
	}
	l.unlock();
	return false;
}

//CHAIN EXCEPTIONS TOGETHER AND SHIFT BUFFERS
bool chainExceptionsAndShift(WTGS_Exception* newExceptions, WTGS_Exception* oldExceptionBuffer, int &newExceptionCount, int &oldExceptionCount, WTGS_Exception* completeExceptions, int &completeExceptionCount)
{
	completeExceptionCount = 0;
	bool oldExceptionIsComplete[100];
	for (int i = 0; i < 100; i++)
	{
		oldExceptionIsComplete[i] = true;
	}

	for (int i = 0; i < newExceptionCount; i++)
	{
		for (int j = 0; j < oldExceptionCount; j++)
		{
			if (newExceptions[i].exceptionType == oldExceptionBuffer[j].exceptionType)
			{
				//WE FOUND A CHAIN-DEFECT, SO THE OLD EXCEPTION IS NOT-YET COMPLETE
				oldExceptionIsComplete[j] = false;

				//INCREMENT EXCEPTION LENGTH
				newExceptions[i].exceptionLength += oldExceptionBuffer[j].exceptionLength;

				//DETERMINE IF MAGNITUDES NEED UPDATING
				if ( (std::abs(newExceptions[i].exceptionAmplitude) < std::abs(oldExceptionBuffer[j].exceptionAmplitude) 
					&& newExceptions[i].exceptionSeverity <= oldExceptionBuffer[j].exceptionSeverity) 
					|| newExceptions[i].exceptionSeverity < oldExceptionBuffer[j].exceptionSeverity)
				{
					newExceptions[i].latitude = oldExceptionBuffer[j].latitude;
					newExceptions[i].longitude = oldExceptionBuffer[j].longitude;
					newExceptions[i].altitude = oldExceptionBuffer[j].altitude;
					newExceptions[i].exceptionAmplitude = oldExceptionBuffer[j].exceptionAmplitude;
					newExceptions[i].exceptionBaseLength = oldExceptionBuffer[j].exceptionBaseLength;
					newExceptions[i].exceptionSeverity = oldExceptionBuffer[j].exceptionSeverity;
					newExceptions[i].max_location = oldExceptionBuffer[j].max_location;
					newExceptions[i].speedTraveling = oldExceptionBuffer[j].speedTraveling;
					newExceptions[i].trackClass = oldExceptionBuffer[j].trackClass;
					newExceptions[i].trackType = oldExceptionBuffer[j].trackType;
					newExceptions[i].secValidity = oldExceptionBuffer[j].secValidity;
				}
				else
				{

				}
			}
		}
	}

	//FILL-UP 'COMPLETE' EXCEPTIONS
	int rollingIndex = 0;
	for (int i = 0; i < oldExceptionCount; i++)
	{
		if (oldExceptionIsComplete[i] == true)
		{
			completeExceptions[rollingIndex] = oldExceptionBuffer[i];
			rollingIndex++;
			completeExceptionCount++;
		}
	}

	memcpy(&oldExceptionBuffer[0], &newExceptions[0], sizeof((WTGS_Exception())) * 100);
	memset(&newExceptions[0], 0x00, sizeof((WTGS_Exception())) * 100);
	oldExceptionCount = newExceptionCount;
	newExceptionCount = 0;
	return true;
}

//FILTERS DATA WINDOW WITH AN FIR FILTER
double firFilter(int coeffIndex, double *data, int windowWidth)
{

	double dataSum = 0;

	for (int i = 0; i < windowWidth; i++)
	{
		if (coeffIndex == 0)
		{
			dataSum += firCoeff_1[i] * data[i];
		}
		if (coeffIndex == 1)
		{
			dataSum += alignmentFirCoeff_1[i] * data[i];
		}
	}

	return dataSum;
}

//CALCULATES THE MEAN VALUE
double meanCalc(int coeffIndex, double *data, int halfWidth1, int halfWidth2)
{
	double cnt = 0;
	double sum = 0;
	for (int i = coeffIndex - halfWidth1; i <= coeffIndex + halfWidth2; i++)
	{
		cnt = cnt + 1;
		sum = sum + data[i];
	}
	double mean = sum / cnt;
	return mean;
}

//CALCULATES THE MEDIAN VALUE ... USE EVEN NUMBER WINDOW..AVERAGES 2 CENTER VALUES
double medianCalc(int coeffIndex, double *data, int halfWidth)
{
	double tmpHolder[200];
	int cnt = 0;
	for (int i = coeffIndex - halfWidth; i <= coeffIndex + halfWidth; i++)
	{
		//COMPARE THE CURRENT INDEX TO VALUES IN TMP HOLDER
		for (int j = 0; j < cnt; j++)
		{
			//SORTING LOWEST TO HIGHTEST ..INDEX 0 = LOWEST INDEX 200 = HIGHEST
			if (data[i] < tmpHolder[j])
			{
				//SHIFT THE ELEMENTS UP
				for (int o = cnt; o >= j; o--)
				{
					tmpHolder[o] = tmpHolder[o - 1];
				}
				//INSERT THE NEW 
				tmpHolder[j] = data[i];
				cnt++;
				//END THE J-LOOP
				j = 9999;
			}
		}

		//IF CNT = 0; INSERT THE VALUE INTO TMP HOLDER
		if (cnt == 0)
		{
			tmpHolder[0] = data[i];
			cnt++;
		}
	}
	double median = (tmpHolder[halfWidth - 1] + tmpHolder[halfWidth]) / 2.0;
	return median; 
}

//CALCULATES THE FRA MEAN -- AVERAGE, ONLY AVERAGING VALUES FALLING ON CORD LENGTH INTERVALS
double fraMeanCalc(int coeffIndex, double *data, int halfWidth1, int halfWidth2, int cordLength)
{
	double cnt = 0; 
	double sum = 0;
	for (int i = coeffIndex - halfWidth1; i <= coeffIndex + halfWidth2; i = i + cordLength)
	{
		cnt = cnt + 1.0;
		sum = sum + data[i];
	}
	double fraMean = sum / cnt;
	return fraMean;
}

//RELATIVE CALCULATION -- REMOVES THE ONGOING 'MEAN' FROM OUTPUT
double relativeCalculation(int coeffIndex, double *data, int halfWidth1, int halfWidth2)
{
	//CALCULATE THE MEAN
	double mean = meanCalc(coeffIndex, data, halfWidth1, halfWidth2);
	double relativeValue = data[coeffIndex] - mean;
	return relativeValue;
}

//DISABLE CALCULATION PROCESSOR
bool GeoCalculations::shutdownGeoCalculations()
{
	calculationProcessorEnable = false;
	return true;
}

//PLACE A NEW EXCEPTION INTO OUTPUT QUE
bool putExceptionOutputQue(WTGS_Exception exception)
{
	std::unique_lock<std::mutex> l(exceptionOutMutex);

	exceptionOutQue.push(exception);
	l.unlock();
	exceptionOutCondVar.notify_one();

	return true;
}

//GET AN EXCEPTION FROM OUTPUT QUE -- USED BY INTERTIALSYSTEM.CPP
bool GeoCalculations::getExceptionOutputQue(WTGS_Exception &exception)
{
	std::unique_lock<std::mutex> l(exceptionOutMutex);
	exceptionOutCondVar.wait_for(l, std::chrono::milliseconds(100), [] {return exceptionOutQue.size() > 0; });

	if (exceptionOutQue.size() > 0)
	{
		exception = exceptionOutQue.front();
		//exceptionOutQue.pop();
		l.unlock();
		return true;
	}
	l.unlock();
	return false;
}


bool GeoCalculations::popExceptionOutputQue()
{
	std::unique_lock<std::mutex> l(exceptionOutMutex);
	exceptionOutCondVar.wait_for(l, std::chrono::milliseconds(100), [] {return exceptionOutQue.size() > 0; });
	if (exceptionOutQue.size() > 0)
	{
		exceptionOutQue.pop();
		l.unlock();
		return true;
	}
	l.unlock();
	return false;
}