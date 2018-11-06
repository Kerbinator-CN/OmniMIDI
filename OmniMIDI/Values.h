// OmniMIDI Values

// Device status
#define DEVICE_UNAVAILABLE 0
#define DEVICE_AVAILABLE 1

// Things
#define MIDI_IO_PACKED 0x00000000L			// Legacy mode, used by all MIDI apps
#define MIDI_IO_COOKED 0x00000002L			// Stream mode, used by some apps (Such as Pinball 3D), NOT SUPPORTED

// path
#define NTFS_MAX_PATH 32767

// Settings managed by client
static BOOL AlreadyStartedOnce = FALSE;

// EVBuffer
struct evbuf_t {
	UINT			uMsg;
	DWORD_PTR		dwParam1;
	DWORD_PTR		dwParam2;
};	// The buffer's structure

static LightweightLock LockSystem;			// LockSystem
static evbuf_t * evbuf;						// The buffer
static volatile ULONGLONG writehead = 0;	// Current write position in the buffer
static volatile ULONGLONG readhead = 0;		// Current read position in the buffer
static volatile LONGLONG eventcount = 0;	// Total events present in the buffer
static ULONGLONG EvBufferSize;
static DWORD EvBufferMultRatio;
static DWORD GetEvBuffSizeFromRAM = 0;

// Device stuff
static DWORD_PTR OMCallback = NULL;
static DWORD_PTR OMInstance = NULL;
static DWORD OMFlags = NULL;
static HDRVR OMDevice = NULL;

// Important stuff
static BOOL DriverInitStatus = FALSE;
static BOOL AlreadyInitializedViaKDMAPI = FALSE;
static BOOL BASSLoadedToMemory = FALSE;
static HANDLE load_sfevent = NULL;
static BOOL ASIOReady = FALSE;
static BOOL EVBuffReady = FALSE;
static BOOL KDMAPIEnabled = FALSE;
static WCHAR SynthNameW[MAXPNAMELEN];		// Synthesizer name

// Stream
static HSTREAM OMStream = NULL;
static BASS_INFO info;
static FLOAT sound_out_volume_float = 1.0;

// GM/GS/XG reset values
static BYTE gs_part_to_ch[16];
static BYTE drum_channels[16];
static const BYTE part_to_ch[16] = { 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15 };
static const char sysex_gm_reset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
static const char sysex_gs_reset[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
static const char sysex_xg_reset[] = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

// Registry system
static LSTATUS KEY_READY = ERROR_SUCCESS;
static LSTATUS KEY_CLOSED = ERROR_INVALID_HANDLE;

typedef struct RegKey
{
	HKEY Address = NULL;
	LSTATUS Status = KEY_CLOSED;
};

static RegKey MainKey, Configuration, Channels, ChanOverride, SFDynamicLoader;

static DWORD Blank = 0;
static DWORD dwType = REG_DWORD, dwSize = sizeof(DWORD);
static DWORD qwType = REG_QWORD, qwSize = sizeof(QWORD);
static DWORD SNType = REG_SZ, SNSize = sizeof(SynthNameW);

// Threads
typedef struct Thread
{
	HANDLE ThreadHandle = NULL;
	ULONG ThreadAddress = NULL;
};

static BOOL bass_initialized = FALSE;
static BOOL block_bassinit = FALSE;
static BOOL stop_thread = FALSE;
static ULONGLONG start1 = 0, start2 = 0, start3 = 0, start4 = 0;
static FLOAT Thread1Usage = 0.0f, Thread2Usage = 0.0f, Thread3Usage = 0.0f, Thread4Usage = 0.0f;

static Thread HealthThread, ATThread, EPThread, DThread;

// Mandatory values
static HINSTANCE hinst = NULL;							// main DLL handle
static HINSTANCE ntdll = NULL;							// ?

static CHAR AppPath[NTFS_MAX_PATH];		// debug info
static TCHAR AppPathW[NTFS_MAX_PATH];	// debug info
static CHAR AppName[MAX_PATH];		// debug info
static TCHAR AppNameW[MAX_PATH];		// debug info
static CHAR bitapp[MAX_PATH];			// debug info
static HANDLE hPipe = INVALID_HANDLE_VALUE;	// debug info

// Main values
static INT AudioOutput = -1;				// Audio output (All devices except AudToWAV and ASIO)
static BASS_FX_VOLUME_PARAM ChVolumeStruct;	// Volume
static HFX ChVolume;						// Volume
static DWORD RestartValue = 0;				// For AudToWAV
static BOOL CloseStreamMidiOutClose = TRUE;	// Close the stream when midiOutClose is called

static HANDLE hConsole;						// Debug console
static FLOAT *sndbf;						// AudToWAV

// Settings and debug
struct SoundFontList
{
	int EnableState;
	wchar_t Path[NTFS_MAX_PATH];
	int SourcePreset;
	int SourceBank;
	int DestinationPreset;
	int DestinationBank;
	int XGBankMode;
} SFLIST, *PSFLIST;

static BOOL SettingsManagedByClient = FALSE;
static FLOAT RenderingTime = 0.0f;
static Settings ManagedSettings = DEFAULT_SETTINGS;
static DebugInfo ManagedDebugInfo = DEFAULT_DEBUG;

// Priority values
static const DWORD prioval[7] =
{
	THREAD_PRIORITY_TIME_CRITICAL,
	THREAD_PRIORITY_TIME_CRITICAL,
	THREAD_PRIORITY_HIGHEST,
	THREAD_PRIORITY_ABOVE_NORMAL,
	THREAD_PRIORITY_NORMAL,
	THREAD_PRIORITY_BELOW_NORMAL,
	THREAD_PRIORITY_HIGHEST
};

// Built-in blacklist
static LPCWSTR BuiltInBlacklist[26] =
{
	_T("Battle.net Launcher.exe"),
	_T("LogonUI.exe"),
	_T("NVDisplay.Container.exe"),
	_T("NVIDIA Share.exe"),
	_T("NVIDIA Web Helper.exe"),
	_T("RustClient.exe"),
	_T("SearchUI.exe"),
	_T("SecurityHealthService.exe"),
	_T("SecurityHealthSystray.exe"),
	_T("ShellExperienceHost.exe"),
	_T("SndVol.exe"),
	_T("WUDFHost.exe"),
	_T("conhost.exe"),
	_T("consent.exe"),
	_T("csrss.exe"),
	_T("ctfmon.exe"),
	_T("dwm.exe"),
	_T("explorer.exe"),
	_T("lsass.exe"),
	_T("mstsc.exe"),
	_T("nvcontainer.exe"),
	_T("nvsphelper64.exe"),
	_T("smss.exe"),
	_T("spoolsv.exe"),
	_T("vcpkgsrv.exe"),
	_T("vmware-hostd.exe")
};

// Channels volume
static LPCWSTR cnames[16] =
{
	L"ch1", L"ch2", L"ch3", L"ch4", L"ch5", L"ch6", L"ch7", L"ch8",
	L"ch9", L"ch10", L"ch11", L"ch12", L"ch13", L"ch14", L"ch15", L"ch16"
};

static DWORD cvvalues[16];

// Channels instruments/banks
static LPCWSTR cbankname[16] =
{
	L"bc1", L"bc2", L"bc3", L"bc4", L"bc5", L"bc6", L"bc7", L"bc8",
	L"bc9", L"bcd", L"bc11", L"bc12", L"bc13", L"bc14", L"bc15", L"bc16"
};

static LPCWSTR cpresetname[16] =
{
	L"pc1", L"pc2", L"pc3", L"pc4", L"pc5", L"pc6", L"pc7", L"pc8",
	L"pc9", L"pcd", L"pc11", L"pc12", L"pc13", L"pc14", L"pc15", L"pc16"
};

static DWORD cbank[16];
static DWORD cpreset[16];

static DWORD SynthType = 2;
static DWORD SynthNamesTypes[7] =
{
	MOD_FMSYNTH,
	MOD_SYNTH,
	MOD_MIDIPORT,
	MOD_WAVETABLE,
	MOD_MAPPER,
	MOD_SWSYNTH,
	MOD_SQSYNTH
};

// Channels
static DWORD cvalues[16];

// Reverb and chorus
static DWORD reverb = 64;					// Reverb
static DWORD chorus = 64;					// Chorus

// Watchdog stuff
static LPCWSTR rnames[16] =
{
	L"rel1", L"rel2", L"rel3", L"rel4", L"rel5", L"rel6", L"rel7", L"rel8",
	L"rel9", L"rel10", L"rel11", L"rel2", L"rel13", L"rel14", L"rel15", L"rel16"
};

static DWORD rvalues[16];

// Other
static DWORD buffull = 0;

// Soundfont lists
static TCHAR userprofile[MAX_PATH];

// -----------------------------------------------------------------------

static TCHAR sfdir1[MAX_PATH];
static TCHAR sfdir2[MAX_PATH];
static TCHAR sfdir3[MAX_PATH];
static TCHAR sfdir4[MAX_PATH];
static TCHAR sfdir5[MAX_PATH];
static TCHAR sfdir6[MAX_PATH];
static TCHAR sfdir7[MAX_PATH];
static TCHAR sfdir8[MAX_PATH];
static TCHAR sfdir9[MAX_PATH];
static TCHAR sfdir10[MAX_PATH];
static TCHAR sfdir11[MAX_PATH];
static TCHAR sfdir12[MAX_PATH];
static TCHAR sfdir13[MAX_PATH];
static TCHAR sfdir14[MAX_PATH];
static TCHAR sfdir15[MAX_PATH];
static TCHAR sfdir16[MAX_PATH];

static TCHAR * sflistloadme[16] =
{
	sfdir1, sfdir2, sfdir3, sfdir4, sfdir5, sfdir6, sfdir7, sfdir8,
	sfdir9, sfdir10, sfdir11, sfdir12, sfdir13, sfdir14, sfdir15, sfdir16
};

static TCHAR * sfdirs[16] =
{
	L"\\OmniMIDI\\lists\\OmniMIDI_A.omlist",
	L"\\OmniMIDI\\lists\\OmniMIDI_B.omlist",
	L"\\OmniMIDI\\lists\\OmniMIDI_C.omlist",
	L"\\OmniMIDI\\lists\\OmniMIDI_D.omlist",
	L"\\OmniMIDI\\lists\\OmniMIDI_E.omlist",
	L"\\OmniMIDI\\lists\\OmniMIDI_F.omlist",
	L"\\OmniMIDI\\lists\\OmniMIDI_G.omlist",
	L"\\OmniMIDI\\lists\\OmniMIDI_H.omlist",
	L"\\OmniMIDI\\lists\\OmniMIDI_I.omlist",
	L"\\OmniMIDI\\lists\\OmniMIDI_L.omlist",
	L"\\OmniMIDI\\lists\\OmniMIDI_M.omlist",
	L"\\OmniMIDI\\lists\\OmniMIDI_N.omlist",
	L"\\OmniMIDI\\lists\\OmniMIDI_O.omlist",
	L"\\OmniMIDI\\lists\\OmniMIDI_P.omlist",
	L"\\OmniMIDI\\lists\\OmniMIDI_Q.omlist",
	L"\\OmniMIDI\\lists\\OmniMIDI_R.omlist"
};

// -----------------------------------------------------------------------

static TCHAR listsloadme1[MAX_PATH];
static TCHAR listsloadme2[MAX_PATH];
static TCHAR listsloadme3[MAX_PATH];
static TCHAR listsloadme4[MAX_PATH];
static TCHAR listsloadme5[MAX_PATH];
static TCHAR listsloadme6[MAX_PATH];
static TCHAR listsloadme7[MAX_PATH];
static TCHAR listsloadme8[MAX_PATH];
static TCHAR listsloadme9[MAX_PATH];
static TCHAR listsloadme10[MAX_PATH];
static TCHAR listsloadme11[MAX_PATH];
static TCHAR listsloadme12[MAX_PATH];
static TCHAR listsloadme13[MAX_PATH];
static TCHAR listsloadme14[MAX_PATH];
static TCHAR listsloadme15[MAX_PATH];
static TCHAR listsloadme16[MAX_PATH];

static TCHAR * listsloadme[16] =
{
	listsloadme1, listsloadme2, listsloadme3, listsloadme4, listsloadme5, listsloadme6, listsloadme7, listsloadme8,
	listsloadme9, listsloadme10, listsloadme11, listsloadme12, listsloadme13, listsloadme14, listsloadme15, listsloadme16
};

static TCHAR * listsanalyze[16] =
{
	L"\\OmniMIDI\\applists\\OmniMIDI_A.applist",
	L"\\OmniMIDI\\applists\\OmniMIDI_B.applist",
	L"\\OmniMIDI\\applists\\OmniMIDI_C.applist",
	L"\\OmniMIDI\\applists\\OmniMIDI_D.applist",
	L"\\OmniMIDI\\applists\\OmniMIDI_E.applist",
	L"\\OmniMIDI\\applists\\OmniMIDI_F.applist",
	L"\\OmniMIDI\\applists\\OmniMIDI_G.applist",
	L"\\OmniMIDI\\applists\\OmniMIDI_H.applist",
	L"\\OmniMIDI\\applists\\OmniMIDI_I.applist",
	L"\\OmniMIDI\\applists\\OmniMIDI_L.applist",
	L"\\OmniMIDI\\applists\\OmniMIDI_M.applist",
	L"\\OmniMIDI\\applists\\OmniMIDI_N.applist",
	L"\\OmniMIDI\\applists\\OmniMIDI_O.applist",
	L"\\OmniMIDI\\applists\\OmniMIDI_P.applist",
	L"\\OmniMIDI\\applists\\OmniMIDI_Q.applist",
	L"\\OmniMIDI\\applists\\OmniMIDI_R.applist"
};

// -----------------------------------------------------------------------

static std::vector<DWORD> _soundFonts;
static std::vector<BASS_MIDI_FONTEX> presetList;

// -----------------------------------------------------------------------

static DWORD pitchshiftchan[16];
static LPCWSTR pitchshiftname[16] =
{
	L"ch1pshift", L"ch2pshift", L"ch3pshift", L"ch4pshift", L"ch5pshift",
	L"ch6pshift", L"ch7pshift", L"ch8pshift", L"ch9pshift", L"ch10pshift",
	L"ch11pshift", L"ch12pshift", L"ch13pshift", L"ch14pshift", L"ch15pshift", 
	L"ch16pshift"
};