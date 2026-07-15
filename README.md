# DataMonitor

JSON 파일을 백엔드로 하는 범용(general-purpose) 데이터 모니터링/CRUD 콘솔
도구 PoC입니다. 자세한 배경과 설계는 `docs/REQUIREMENTS.md`,
`docs/ARCHITECTURE.md`를 참고하세요.

## 구성

- `DataMonitorCore` - 재사용 가능한 JSON 기반 "임베디드 DB" 엔진 (StaticLibrary)
- `DataMonitor` - `DataMonitorCore`를 사용하는 콘솔 데모 앱 (반도체 샘플/발주 더미 데이터로 시연)
- `DataMonitorTests` - 자체 제작한 마이크로 테스트 프레임워크로 작성한 단위 테스트

## Visual Studio에서 빌드/실행

1. `DataMonitor.slnx`를 Visual Studio 2026 (v18)에서 엽니다.
2. 솔루션 구성(Debug/Release)과 플랫폼(x64/x86)을 선택 후 빌드합니다.
3. 시작 프로젝트를 `DataMonitor`로 설정하고 실행(F5)하면 콘솔 메뉴가 뜹니다.
4. `DataMonitorTests`를 시작 프로젝트로 설정해 실행하면 단위 테스트 결과가
   콘솔에 출력됩니다 (실패 시 종료 코드가 0이 아님).

## 커맨드라인 빌드 (MSBuild)

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" `
  DataMonitor.slnx /p:Configuration=Debug /p:Platform=x64
```

빌드 산출물은 `x64\Debug\` (또는 `x64\Release\`, `Win32\...` 등) 아래에
`DataMonitor.exe`, `DataMonitorTests.exe`, `DataMonitorCore.lib`로 생성됩니다.

## 실행

```powershell
cd x64\Debug
.\DataMonitorTests.exe   # 단위 테스트 실행, 마지막에 Pass/Fail 요약 출력
.\DataMonitor.exe        # 콘솔 데모 앱 실행 (최초 실행 시 .\data 폴더에 더미 데이터 생성)
```

`DataMonitor.exe`는 실행 디렉터리 기준 `.\data\` 폴더에 테이블별 JSON 파일
(`samples.json`, `orders.json` 등)을 생성/사용합니다.

## 외부 의존성

없습니다. vcpkg/NuGet 등 패키지 매니저 없이, MSVC 표준 라이브러리
(`<filesystem>` 포함)만으로 빌드됩니다.

## `data/` 디렉터리에 대한 정책

`.gitignore`에서 `data/`는 **제외하지 않습니다** (즉, 커밋 대상으로 둘 수
있습니다). 이 프로젝트는 "JSON 파일이 곧 DB"이므로, 작은 시드 데이터셋을
저장소에 함께 커밋해 두면 클론 직후에도 바로 조회/검색을 시연할 수 있다는
장점이 있습니다. 다만 실제로 커밋할지 여부는 사용자의 선택이며, 데이터가
민감하거나 커진다면 `data/`도 얼마든지 무시 목록에 추가해도 됩니다.
