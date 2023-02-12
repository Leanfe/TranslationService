#include <Windows.h>
#include <iostream>
#include <string>
#include <future>
#include <thread>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <Windows.h>

using json = nlohmann::json;

std::string language;

void ReadLanguageFromFile()
{
  std::string homeDir = getenv("USERPROFILE");
  std::string filePath = homeDir + "\\language.txt";
  std::ifstream file(filePath);

  if (file.is_open())
  {
    std::getline(file, language);
    file.close();
  }
  else
  {
    // Show native window to enter the language code
    std::string message = "Enter the language code (e.g. en, fr, de, etc.):";
    std::string title = "Select Language";
    char buffer[100];
    int result = MessageBoxA(NULL, message.c_str(), title.c_str(), MB_OKCANCEL | MB_ICONINFORMATION);
    if (result == IDOK)
    {
      std::cin >> buffer;
      language = buffer;
    }
    else
    {
      exit(0);
    }
  }
}

class Translator {
    public:
    std::string target_language = std::move(language);

    std::string Translate(const std::string& text) {
        CURL *curl = curl_easy_init();
        if (!curl) {
        std::cerr << "cURL initialization failed" << std::endl;
        return "";
        }

        std::string url = "https://api.example.com/translate?text=" + text + "&target=" + target_language;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
        std::cerr << "cURL request failed: " << curl_easy_strerror(res) << std::endl;
        return "";
        }

        long response_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code != 200) {
        std::cerr << "API request failed: " << response_code << std::endl;
        return "";
        }

        curl_easy_cleanup(curl);

        // Parse the response to get the translated text
        json response_json = json::parse(response);
        std::string translated_text = response_json["translatedText"];

        return translated_text;
    }

    private:
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);

        return size * nmemb;
    }

  std::string response;
};

HHOOK hHook = NULL;
Translator translator;
std::future<std::string> result;
std::string text_to_translate;
bool started_translation = false;
bool is_typing = false;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        switch (wParam) {
            case WM_KEYDOWN:
                switch (p->vkCode) {
                    case VK_F5:
                        std::cout << "Starting Translation..." << std::endl;
                        started_translation = true;
                        text_to_translate.clear();
                        is_typing = false;
                        break;
                    case VK_RETURN:
                        if (started_translation) {
                            result = std::async(std::launch::async, &Translator::Translate, &translator, text_to_translate);
                            started_translation = false;
                        } else if (result.valid() && result.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                            std::string translated_text = result.get();
                            std::cout << "Translated Text: " << translated_text << std::endl;
                    
                        // Send the keystrokes corresponding to the translated text
                        for (char c : translated_text) {
                            INPUT input[2] = { 0 };
                            input[0].type = INPUT_KEYBOARD;
                            input[0].ki.wVk = 0;
                            input[0].ki.wScan = c;
                            input[0].ki.dwFlags = KEYEVENTF_UNICODE;
                            input[1].type = INPUT_KEYBOARD;
                            input[1].ki.wVk = 0;
                            input[1].ki.wScan = c;
                            input[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
                            SendInput(2, input, sizeof(INPUT));
                        }

                        // Reset the translation result
                        result = std::future<std::string>();
                    }
                    break;
                default:
                    if (started_translation) {
                        char key = MapVirtualKey(p->vkCode, MAPVK_VK_TO_CHAR);
                        text_to_translate += key;
                    } else if (!is_typing) {
                        // Cancel the keystroke
                        is_typing = true;
                        return 1;
                    }
                    break;
                }
        break;
        }
    }
  return CallNextHookEx(hHook, nCode, wParam, lParam);
}

int main() {
  ReadLanguageFromFile();

  std::cout << "Press F5 to start the translation" << std::endl;
  std::cout << "Press Enter to get the translated text" << std::endl;

  hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
  if (hHook == NULL) {
    std::cerr << "Error setting hook" << std::endl;
    return 1;
  }

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  UnhookWindowsHookEx(hHook);
  return 0;
}