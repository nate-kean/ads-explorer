#include "pch.h"
#include "CppUnitTest.h"
#include <shtypes.h>
#include <comutil.h>

#include "EnumIDList.h"
#include "defer.h"

#include <stdexcept>


using namespace Microsoft::VisualStudio::CppUnitTestFramework;


static const _bstr_t bstrWorkingDir =
	"G:\\Garlic\\Documents\\Code\\Visual Studio\\ADS Explorer Saga\\"
	"ADS Explorer\\Test\\Files\\";


static CComObject<CEnumIDList> *make_enumerator(const BSTR &sPath) {
	CComObject<CEnumIDList> *pEnum;
	HRESULT hr = CComObject<CEnumIDList>::CreateInstance(&pEnum);
	if (FAILED(hr)) {
		throw std::runtime_error("Failed to create enumerator instance");
	}
	pEnum->AddRef();
	pEnum->Init(pEnum->GetUnknown(), sPath);
	return pEnum;
}


static void DoTest(
	_bstr_t bstrFSObjName,
	ULONG cAvailable,
	ULONG cRequested,
	HRESULT hrExpected
) {
	bstrFSObjName = bstrWorkingDir + bstrFSObjName;
	CComObject<CEnumIDList> *pEnum = make_enumerator(bstrFSObjName.Detach());
	defer({ pEnum->Release(); });

	auto pidls = new PITEMID_CHILD[cRequested];
	defer({ delete[] pidls; });
	ULONG totalCollected = 0;

	HRESULT hr = pEnum->Next(cRequested, pidls, &totalCollected);

	Assert::AreEqual(hr, hrExpected);
	Assert::AreEqual(totalCollected, min(cAvailable, cRequested));
}


namespace Test {
	TEST_CLASS(TestCEnumIDList) {
	  public:
		TEST_METHOD(Test0Streams0Requested) {
			DoTest("0streams", 0, 0, S_OK);
		}
		TEST_METHOD(Test0Streams1Requested) {
			DoTest("0streams", 0, 1, S_FALSE);
		}
		TEST_METHOD(Test0Streams2Requested) {
			DoTest("0streams", 0, 2, S_FALSE);
		}
		TEST_METHOD(Test0Streams3Requested) {
			DoTest("0streams", 0, 3, S_FALSE);
		}

		TEST_METHOD(Test1Stream0Requested) {
			DoTest("1stream.txt", 1, 0, S_OK);
		}
		TEST_METHOD(Test1Stream1Requested) {
			DoTest("1stream.txt", 1, 1, S_OK);
		}
		TEST_METHOD(Test1Stream2Requested) {
			DoTest("1stream.txt", 1, 2, S_FALSE);
		}
		TEST_METHOD(Test1Stream3Requested) {
			DoTest("1stream.txt", 1, 3, S_FALSE);
		}

		TEST_METHOD(Test2Streams0Requested) {
			DoTest("2streams.txt", 2, 0, S_OK);
		}
		TEST_METHOD(Test2Streams1Requested) {
			DoTest("2streams.txt", 2, 1, S_OK);
		}
		TEST_METHOD(Test2Streams2Requested) {
			DoTest("2streams.txt", 2, 2, S_OK);
		}
		TEST_METHOD(Test2Streams3Requested) {
			DoTest("2streams.txt", 2, 3, S_FALSE);
		}

		TEST_METHOD(Test3Streams0Requested) {
			DoTest("3streams.txt", 3, 0, S_OK);
		}
		TEST_METHOD(Test3Streams1Requested) {
			DoTest("3streams.txt", 3, 1, S_OK);
		}
		TEST_METHOD(Test3Streams2Requested) {
			DoTest("3streams.txt", 3, 2, S_OK);
		}
		TEST_METHOD(Test3Streams3Requested) {
			DoTest("3streams.txt", 3, 3, S_OK);
		}
	};
}
