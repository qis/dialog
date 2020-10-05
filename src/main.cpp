#include <version.h>

class Window {
public:
  static constexpr PCWSTR title_text = TEXT(PROJECT_DESCRIPTION);
  static constexpr PCWSTR class_name = TEXT(PROJECT_DESCRIPTION PROJECT_VERSION);

  Window() noexcept : hinstance_(GetModuleHandle(nullptr)) {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_NOCLOSE;
    wc.lpfnWndProc = Proc;
    wc.hInstance = hinstance_;
    wc.hIcon = wc.hIconSm = LoadIcon(hinstance_, MAKEINTRESOURCE(101));
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOWFRAME);
    wc.lpszClassName = class_name;
    RegisterClassEx(&wc);
    const auto ws = WS_CAPTION;
    const auto ex = WS_EX_APPWINDOW;
    CreateWindowEx(ex, class_name, title_text, ws, 0, 0, 640, 480, nullptr, nullptr, hinstance_, this);
  }

  Window(Window&& other) = delete;
  Window(const Window& other) = delete;
  Window& operator=(Window&& other) = delete;
  Window& operator=(const Window& other) = delete;

  ~Window() {
    UnregisterClass(class_name, hinstance_);
  }

  winrt::Windows::Foundation::IAsyncAction Pick() {
    try {
      winrt::Windows::Storage::Pickers::FileOpenPicker picker;
      picker.as<IInitializeWithWindow>()->Initialize(hwnd_);

      picker.SuggestedStartLocation(winrt::Windows::Storage::Pickers::PickerLocationId::Desktop);
      picker.FileTypeFilter().Append(L".jpg");

      const auto file = co_await picker.PickSingleFileAsync();
      if (file) {
        const auto path = file.Path();
        MessageBoxW(nullptr, path.data(), L"File", MB_OK | MB_SETFOREGROUND);
      }
    }
    catch (const winrt::hresult_error& e) {
      const auto message = e.message();
      MessageBoxW(nullptr, message.data(), L"Error", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
    }
    PostMessage(hwnd_, WM_CLOSE, 0, 0);
    co_return;
  }

  void OnCreate() noexcept {
    Pick();
  }

  void OnDestroy() noexcept {
    UnregisterHotKey(hwnd_, 0);
    PostQuitMessage(0);
  }

  int Run() noexcept {
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
      DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
  }

private:
  static LRESULT __stdcall Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept {
    switch (msg) {
    case WM_CREATE:
      if (const auto window = reinterpret_cast<Window*>(reinterpret_cast<LPCREATESTRUCT>(lparam)->lpCreateParams)) {
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        window->hwnd_ = hwnd;
        window->OnCreate();
      }
      return 0;
    case WM_DESTROY:
      if (const auto window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA))) {
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        window->OnDestroy();
      }
      return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
  }

  HINSTANCE hinstance_;
  HWND hwnd_ = nullptr;
};

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR cmd, int show) {
  winrt::init_apartment();
  Window window;
  return window.Run();
}
