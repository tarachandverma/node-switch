var switch_bindings = require('../build/Release/switch_bindings');

exports.initialize = function(configFile, callback) {
  switch_bindings.initialize(configFile, callback);
} 

exports.isSwitchEnabled = function(moduleName, switchName, switchValue) {
  return switch_bindings.isSwitchEnabled(moduleName, switchName, switchValue);
}
