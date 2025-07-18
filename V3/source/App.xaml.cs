using System;
using System.IO;
using System.Windows;

namespace ps2batchrenamer
{
    public partial class App : Application
    {
        private const string LogFile = "ps2batchrenamer_startup.log";

        protected override void OnStartup(StartupEventArgs e)
        {
            try
            {
                AppDomain.CurrentDomain.UnhandledException += CurrentDomain_UnhandledException;
                DispatcherUnhandledException += App_DispatcherUnhandledException;

                Log("Application starting...");

                base.OnStartup(e);
            }
            catch (Exception ex)
            {
                Log($"Exception in OnStartup: {ex}");
                throw;
            }
        }

        private void App_DispatcherUnhandledException(object sender, System.Windows.Threading.DispatcherUnhandledExceptionEventArgs e)
        {
            Log($"DispatcherUnhandledException: {e.Exception}");
            e.Handled = false;
        }

        private void CurrentDomain_UnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            Log($"CurrentDomain_UnhandledException: {e.ExceptionObject}");
        }

        public static void Log(string message)
        {
            try
            {
                string path = Path.Combine(Path.GetTempPath(), LogFile);
                File.AppendAllText(path, $"{DateTime.Now:yyyy-MM-dd HH:mm:ss} - {message}{Environment.NewLine}");
            }
            catch
            {
                // Ignore failures in logging
            }
        }
    }
}
