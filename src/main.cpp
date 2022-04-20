#include <cstdlib>
#include <cstring>
#include <vector>
#include <unordered_map>
#include "napi.h"
#include "script.h"
#include "base64.h"

#include "aes/aes.hpp"

#define KEY_LENGTH 32

#define FN_MODULE_PROTOTYPE__COMPILE 0

namespace {

struct AddonData {
  std::unordered_map<int, Napi::FunctionReference> functions;
};

const char errmsg[] = "This program has been changed by others.";

/* void ConsoleLog(const Napi::Env& env, Napi::Value value) {
  Napi::Object console = env.Global().As<Napi::Object>()
    .Get("console").As<Napi::Object>();
  Napi::Function log = console.Get("log").As<Napi::Function>();
  log.Call(console, { value });
} */

void ConsoleError(const Napi::Env& env, Napi::Value value) {
  Napi::Object console = env.Global().As<Napi::Object>()
    .Get("console").As<Napi::Object>();
  Napi::Function error = console.Get("error").As<Napi::Function>();
  error.Call(console, { value });
}

const uint8_t* GetKey() {
  static const uint8_t key[KEY_LENGTH] = {
#include "key.txt"
  };

  return key;
}

int Pkcs7cut(uint8_t *p, int plen) {
  uint8_t last = p[plen - 1];
  if (last > 0 && last <= 16) {
    for (int x = 2; x <= last; x++) {
      if (p[plen - x] != last) {
        return plen;
      }
    }
    return plen - last;
  }

  return plen;
}

std::string Aesdec(const std::vector<uint8_t>& data,
                   const uint8_t* key,
                   const uint8_t* iv) {
  size_t l = data.size();
  uint8_t* encrypt = new uint8_t[l];
  memcpy(encrypt, data.data(), l);

  struct AES_ctx ctx;
  AES_init_ctx_iv(&ctx, key, iv);
  AES_CBC_decrypt_buffer(&ctx, encrypt, l);

  uint8_t* out = new uint8_t[l + 1];
  memcpy(out, encrypt, l);
  out[l] = 0;

  int realLength = Pkcs7cut(out, l);
  out[realLength] = 0;

  delete[] encrypt;
  std::string res = reinterpret_cast<char*>(out);
  delete[] out;
  return res;
}

std::string Decrypt(const std::string& base64) {
  size_t buflen = base64_decode(base64.c_str(), base64.length(), nullptr);
  if (buflen == 0) return "";
  std::vector<uint8_t> buf(buflen);
  base64_decode(base64.c_str(), base64.length(), &buf[0]);

  std::vector<uint8_t> iv(buf.begin(), buf.begin() + 16);
  std::vector<uint8_t> data(buf.begin() + 16, buf.end());

  return Aesdec(data, GetKey(), iv.data());
}

Napi::Value ModulePrototypeCompile(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  AddonData* addon_data = static_cast<AddonData*>(info.Data());
  Napi::String content = info[0].As<Napi::String>();
  Napi::String filename = info[1].As<Napi::String>();
  std::string filename_str = filename.Utf8Value();
  Napi::Function old_compile =
    addon_data->functions[FN_MODULE_PROTOTYPE__COMPILE].Value();

#if !defined(JS_ASAR_CONFIG_NO_ASAR) || JS_ASAR_CONFIG_NO_ASAR == 0
  if (filename_str.find("app.asar") != std::string::npos) {
#endif

    return old_compile.Call(info.This(),
      { Napi::String::New(env, Decrypt(content.Utf8Value())), filename });

#if !defined(JS_ASAR_CONFIG_NO_ASAR) || JS_ASAR_CONFIG_NO_ASAR == 0
  }
  return old_compile.Call(info.This(), { content, filename });
#endif
}

Napi::Function MakeRequireFunction(Napi::Env* env,
                                   const Napi::Object& mod) {
  Napi::Function make_require = env->RunScript(scriptRequire)
    .As<Napi::Function>();
  return make_require({ mod }).As<Napi::Function>();
}

Napi::Value GetModuleObject(Napi::Env* env,
                            const Napi::Object& main_module,
                            const Napi::Object& this_exports) {
  Napi::Function find_function = env->RunScript(scriptFind)
    .As<Napi::Function>();
  Napi::Value res = find_function({ main_module, this_exports });
  if (res.IsNull()) {
    Napi::Error::New(*env, "Cannot find module object.")
      .ThrowAsJavaScriptException();
  }
  return res;
}

void ShowErrorAndQuit(const Napi::Env& env,
                      const Napi::String& message) {
  ConsoleError(env, message);
  exit(1);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  Napi::Object process = env.Global().Get("process").As<Napi::Object>();
  Napi::Array argv = process.Get("argv").As<Napi::Array>();
  for (uint32_t i = 0; i < argv.Length(); ++i) {
    std::string arg = argv.Get(i).As<Napi::String>().Utf8Value();
    if (arg.find("--inspect") == 0 ||
        arg.find("--remote-debugging-port") == 0) {
      Napi::Error::New(env, "Not allow debugging this program.")
        .ThrowAsJavaScriptException();
      return exports;
    }
  }
  Napi::Object main_module = process.Get("mainModule").As<Napi::Object>();

  Napi::Object this_module = GetModuleObject(&env, main_module, exports)
    .As<Napi::Object>();
  Napi::Function require = MakeRequireFunction(&env, this_module);

  Napi::Object module_constructor = require({
    Napi::String::New(env, "module") }).As<Napi::Object>();

  Napi::Value module_parent = this_module.Get("parent");

  if (this_module != main_module || (
    module_parent != module_constructor &&
    module_parent != env.Undefined() &&
    module_parent != env.Null())) {
    ShowErrorAndQuit(env, Napi::String::New(env, errmsg));
    return exports;
  }

  AddonData* addon_data = env.GetInstanceData<AddonData>();

  if (addon_data == nullptr) {
    addon_data = new AddonData();
    env.SetInstanceData(addon_data);
  }

  Napi::Object module_prototype = module_constructor.Get("prototype")
    .As<Napi::Object>();
  addon_data->functions[FN_MODULE_PROTOTYPE__COMPILE] =
    Napi::Persistent(module_prototype.Get("_compile").As<Napi::Function>());

  module_prototype.DefineProperty(
    Napi::PropertyDescriptor::Function(env,
      Napi::Object::New(env),
      "_compile",
      ModulePrototypeCompile,
      napi_enumerable,
      addon_data));

#if !defined(JS_ASAR_CONFIG_NO_ASAR) || JS_ASAR_CONFIG_NO_ASAR == 0
  Napi::Function register_asar_fn = env.RunScript(scriptAsarNode)
    .As<Napi::Function>();
  register_asar_fn({ require });
#endif

  try {
    require({ Napi::String::New(env, JS_ASAR_REQUIRE_FROM_MAIN) });
  } catch (const Napi::Error& e) {
    ShowErrorAndQuit(env, e.Get("stack").As<Napi::String>());
  }
  return exports;
}

}  // namespace

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
