# Configuration Management - Quick Reference

## 🚀 Quick Start (30 seconds)

```cpp
#include "engine/config/ConfigEditor.h"

ConfigEditor editor;
editor.OpenConfig("assets/actors/ships/player.json");
editor.SetFieldValue("health", simplejson::JsonValue(1500.0));
if (editor.ValidateAll().valid) {
    editor.SaveConfig();
}
```

---

## 📋 Common Tasks

### Open and Edit Configuration

```cpp
ConfigEditor editor;
editor.OpenConfig("path/to/config.json");
editor.SetFieldValue("fieldName", value);
editor.SaveConfig();
```

### Real-Time Validation

```cpp
RealTimeValidator validator;
validator.StartValidation("config.json");
validator.AddListener([](const ValidationResult& r) {
    if (!r.valid) std::cout << "Invalid!" << std::endl;
});
validator.ValidateIncremental("field", value);
```

### Use Template

```cpp
auto& mgr = ConfigTemplateManager::GetInstance();
std::unordered_map<std::string, simplejson::JsonValue> params;
params["SHIP_NAME"] = simplejson::JsonValue("Fighter");
auto config = mgr.InstantiateTemplate("BasicShip", params);
```

### Run Tests

```cpp
ConfigTestSuite suite("Tests");
suite.AddTest("Check", "Description", [](const auto& cfg) {
    return /* validation logic */;
});
auto& runner = ConfigTestRunner::GetInstance();
runner.RegisterSuite("Type", suite);
auto report = runner.RunTests("Type", "config.json");
```

### Deploy Configuration

```cpp
auto& deploy = ConfigDeployment::GetInstance();
DeploymentOptions opts;
opts.target = DeploymentTarget::Production;
opts.validateBeforeDeploy = true;
auto result = deploy.Deploy("config.json", opts);
```

### Enable Caching

```cpp
auto& cache = ConfigManager::GetInstance().GetCache();
cache.SetCachePolicy(CachePolicy::LRU, 100); // 100MB
cache.Preload({"config1.json", "config2.json"});
auto stats = cache.GetStats();
```

### Track Analytics

```cpp
auto& analytics = ConfigManager::GetInstance().GetAnalytics();
auto stats = analytics.GetStats("config.json");
auto mostUsed = analytics.GetMostUsed(10);
auto unused = analytics.FindUnusedConfigs(30);
```

### Generate Documentation

```cpp
ConfigDocumentation::DocOptions opts;
opts.format = ConfigDocumentation::DocFormat::Markdown;
auto doc = ConfigDocumentation::GenerateDocumentation("Type", opts);
ConfigDocumentation::ExportDocumentation("Type", "out.md", opts);
```

---

## 🎯 Key Classes

| Class | Purpose | Header |
|-------|---------|--------|
| `ConfigEditor` | Visual configuration editor | `ConfigEditor.h` |
| `RealTimeValidator` | Live validation | `ConfigEditor.h` |
| `ConfigTemplateManager` | Template system | `ConfigEditor.h` |
| `ConfigTestRunner` | Automated testing | `ConfigEditor.h` |
| `ConfigDeployment` | Deployment pipeline | `ConfigEditor.h` |
| `ConfigDocumentation` | Doc generation | `ConfigEditor.h` |
| `ConfigManager` | Main config system | `ConfigManager.h` |
| `ConfigCache` | Performance cache | `ConfigManager.h` |
| `ConfigAnalytics` | Usage analytics | `ConfigManager.h` |
| `ConfigSecurity` | Security features | `ConfigManager.h` |

---

## 📁 File Locations

```
assets/
├── actors/               # Actor configurations
│   ├── ships/           # Ship configs
│   ├── world/           # World object configs
│   └── projectiles/     # Projectile configs
├── templates/           # Configuration templates
│   ├── ship_basic.json
│   ├── ship_fighter.json
│   ├── ship_trader.json
│   └── station_basic.json
└── schemas/             # JSON schemas (if used)

docs/
├── CONFIGURATION_MANAGEMENT.md  # Full documentation
├── CONFIG_SYSTEM_STATUS.md      # Implementation status
└── CONFIG_QUICK_REFERENCE.md    # This file

examples/
└── config_management_example.cpp # Complete examples
```

---

## ⚡ Performance Tips

1. **Enable Caching:** `cache.SetCachePolicy(CachePolicy::LRU, 100)`
2. **Preload Configs:** `cache.Preload(frequentConfigs)`
3. **Batch Operations:** Use `DeployBatch()` instead of multiple `Deploy()`
4. **Incremental Validation:** Use `ValidateIncremental()` for single fields
5. **Cache Results:** Real-time validator caches validation results

---

## 🔒 Security Checklist

- [ ] Enable signature validation: `options.validateSignatures = true`
- [ ] Encrypt sensitive fields: `ConfigSecurity::EncryptSensitiveFields()`
- [ ] Sanitize input: `options.sanitizeInput = true`
- [ ] Validate before deployment: `options.validateBeforeDeploy = true`
- [ ] Use secure encryption keys
- [ ] Backup before changes: `options.backupExisting = true`

---

## 🧪 Testing Workflow

```cpp
// 1. Create test suite
ConfigTestSuite suite("MyTests");

// 2. Add tests
suite.AddTest("Test Name", "Description", [](const auto& cfg) {
    return /* validation */;
});

// 3. Register suite
ConfigTestRunner::GetInstance().RegisterSuite("ConfigType", suite);

// 4. Run tests
auto report = ConfigTestRunner::GetInstance().RunTests("ConfigType", "config.json");

// 5. Check results
if (report.AllPassed()) {
    std::cout << "✓ All tests passed!" << std::endl;
}
```

---

## 🚀 Deployment Workflow

```cpp
// 1. Configure options
DeploymentOptions opts;
opts.target = DeploymentTarget::Production;
opts.validateBeforeDeploy = true;
opts.backupExisting = true;
opts.runTests = true;
opts.dryRun = false;  // Set true for testing

// 2. Set hooks (optional)
deploy.SetDeploymentHook(
    [](const std::string& path) { /* pre-deploy */ return true; },
    [](const DeploymentResult& r) { /* post-deploy */ }
);

// 3. Deploy
auto result = deploy.Deploy("config.json", opts);

// 4. Check result
if (result.success) {
    std::cout << "Deployed in " << result.deploymentDurationMs << "ms" << std::endl;
} else {
    std::cerr << "Failed: " << result.message << std::endl;
}
```

---

## 📊 Analytics Dashboard

```cpp
auto& analytics = ConfigManager::GetInstance().GetAnalytics();

// Usage statistics
auto stats = analytics.GetStats("config.json");
std::cout << "Loads: " << stats.loadCount << std::endl;
std::cout << "Avg Load Time: " << stats.avgLoadTimeMs << "ms" << std::endl;

// Most used configs
auto mostUsed = analytics.GetMostUsed(10);
for (const auto& stat : mostUsed) {
    std::cout << stat.configPath << ": " << stat.loadCount << " loads" << std::endl;
}

// Optimization opportunities
auto unused = analytics.FindUnusedConfigs(30);  // 30 days
auto slowest = analytics.GetSlowestLoading(10);

// Export report
analytics.ExportReport("analytics_report.json");
```

---

## 🎨 Custom Field Types

```cpp
EditorField field;
field.id = "fieldName";
field.label = "Display Label";
field.type = EditorFieldType::Number;  // Or Text, Boolean, Slider, etc.
field.required = true;
field.minValue = 0.0;
field.maxValue = 100.0;
field.defaultValue = simplejson::JsonValue(50.0);
field.tooltip = "Help text";
field.category = "General";
```

**Available Field Types:**
- `Text` - String input
- `Number` - Numeric input
- `Boolean` - Checkbox
- `Slider` - Range slider
- `Dropdown` - Selection list
- `Color` - Color picker
- `Vector2` - 2D vector
- `Vector3` - 3D vector
- `FileSelect` - File browser
- `TextArea` - Multi-line text
- `JsonObject` - Nested object
- `JsonArray` - Array editor

---

## 🔧 Troubleshooting

### Configuration Won't Load
```cpp
// Check if file exists
if (!std::filesystem::exists(path)) {
    std::cerr << "File not found: " << path << std::endl;
}

// Check JSON syntax
auto result = simplejson::Parse(content);
if (!result.success) {
    std::cerr << "Parse error: " << result.errorMessage << std::endl;
}
```

### Validation Fails
```cpp
auto validation = editor.ValidateAll();
if (!validation.valid) {
    for (const auto& error : validation.errors) {
        std::cerr << "Error at " << error.path << ": " 
                 << error.message << std::endl;
    }
}
```

### Deployment Issues
```cpp
// Use dry run first
DeploymentOptions opts;
opts.dryRun = true;
auto result = deploy.Deploy("config.json", opts);

// Check pre-deploy validation
if (!deploy.ValidateDeployment("config.json", opts)) {
    std::cerr << "Pre-deployment validation failed" << std::endl;
}
```

### Performance Issues
```cpp
// Check cache statistics
auto stats = cache.GetStats();
std::cout << "Hit Rate: " << stats.hitRate << "%" << std::endl;

// If hit rate low, increase cache size or preload more configs
cache.SetCachePolicy(CachePolicy::LRU, 200);  // Increase to 200MB
cache.Preload(frequentConfigs);
```

---

## 📚 Further Reading

- **Full Documentation:** `docs/CONFIGURATION_MANAGEMENT.md`
- **Implementation Status:** `docs/CONFIG_SYSTEM_STATUS.md`
- **Code Examples:** `examples/config_management_example.cpp`
- **API Headers:** `engine/config/*.h`
- **Templates:** `assets/templates/*.json`

---

## 💡 Pro Tips

1. **Auto-save:** `editor.EnableAutoSave(60)` - Save every 60 seconds
2. **Undo/Redo:** Use `editor.Undo()` and `editor.Redo()`
3. **Callbacks:** Set validation and change callbacks for real-time feedback
4. **Batch Testing:** Test entire directories with `RunTestsOnDirectory()`
5. **Template Tags:** Use tags for better template organization
6. **Cache Preloading:** Preload configs at startup for better performance
7. **Dry Runs:** Always test deployments with `dryRun = true` first
8. **Analytics:** Review analytics regularly to optimize config usage

---

## 🆘 Support

For issues or questions:
1. Check full documentation: `docs/CONFIGURATION_MANAGEMENT.md`
2. Review examples: `examples/config_management_example.cpp`
3. Check implementation status: `docs/CONFIG_SYSTEM_STATUS.md`
4. Review API headers for detailed documentation

---

**Quick Reference Version:** 1.0.0  
**Last Updated:** 2024
