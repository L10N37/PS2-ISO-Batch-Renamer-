using System;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Windows;
using Ookii.Dialogs.Wpf;

namespace ps2batchrenamer
{
    public partial class MainWindow : Window
    {
        private string? selectedFolder;

        public MainWindow()
        {
            InitializeComponent();

            // Hook checkbox events after all controls initialized
            UseEmbeddedZipCheckbox.Checked += UseEmbeddedZipCheckbox_Checked;
            UseEmbeddedZipCheckbox.Unchecked += UseEmbeddedZipCheckbox_Unchecked;

            UpdateZipControls();

            AppendStatus("Application started. Running diagnostics...");
            RunDiagnostics();
        }

        private void RunDiagnostics()
        {
            try
            {
                AppendStatus("Embedded resources found:");
                var asm = Assembly.GetExecutingAssembly();
                var resources = asm.GetManifestResourceNames();

                foreach (var res in resources)
                {
                    AppendStatus($"- {res}");
                }

                string resourceName = "ps2batchrenamer.ps2_batch_renamer.zip";
                using Stream? resourceStream = asm.GetManifestResourceStream(resourceName);
                if (resourceStream != null)
                {
                    AppendStatus($"Embedded ZIP resource '{resourceName}' loaded successfully, size: {resourceStream.Length} bytes.");
                }
                else
                {
                    AppendStatus($"ERROR: Embedded ZIP resource '{resourceName}' not found!");
                }
            }
            catch (Exception ex)
            {
                AppendStatus($"Diagnostics error: {ex.Message}");
            }
        }

        private void BtnSelectFolder_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new System.Windows.Forms.FolderBrowserDialog();
            var result = dialog.ShowDialog();

            if (result == System.Windows.Forms.DialogResult.OK)
            {
                selectedFolder = dialog.SelectedPath;
                AppendStatus($"Selected folder: {selectedFolder}");
            }
        }

        private void BtnBrowseZip_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new VistaOpenFileDialog
            {
                Title = "Select ZIP File",
                Filter = "ZIP Files (*.zip)|*.zip|All Files (*.*)|*.*"
            };

            bool? result = dlg.ShowDialog(this);
            if (result == true)
            {
                ZipFilePathTextBox.Text = dlg.FileName;
                AppendStatus($"Selected ZIP file: {dlg.FileName}");
            }
        }

        private void BtnRunRenamer_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrEmpty(selectedFolder))
            {
                AppendStatus("Please select a folder first.");
                return;
            }

            BtnSelectFolder.IsEnabled = false;
            BtnRunRenamer.IsEnabled = false;
            BtnBrowseZip.IsEnabled = false;
            UseEmbeddedZipCheckbox.IsEnabled = false;

            try
            {
                AppendStatus("Starting renamer process...");

                string zipTargetPath = Path.Combine(selectedFolder, "ps2_batch_renamer.zip");

                if (UseEmbeddedZipCheckbox.IsChecked == true)
                {
                    AppendStatus("Using embedded ZIP...");

                    string resourceName = "ps2batchrenamer.ps2_batch_renamer.zip";

                    using Stream? resourceStream = Assembly.GetExecutingAssembly().GetManifestResourceStream(resourceName);
                    if (resourceStream == null)
                    {
                        AppendStatus($"ERROR: Embedded ZIP resource not found: {resourceName}");
                        return;
                    }

                    using FileStream fs = new FileStream(zipTargetPath, FileMode.Create, FileAccess.Write);
                    resourceStream.CopyTo(fs);
                }
                else
                {
                    string externalZipPath = ZipFilePathTextBox.Text.Trim();
                    if (!File.Exists(externalZipPath))
                    {
                        AppendStatus($"ERROR: External ZIP file not found: {externalZipPath}");
                        return;
                    }

                    AppendStatus("Copying external ZIP to selected folder...");
                    File.Copy(externalZipPath, zipTargetPath, overwrite: true);
                }

                AppendStatus("Extracting ZIP...");
                ZipFile.ExtractToDirectory(zipTargetPath, selectedFolder, overwriteFiles: true);
                AppendStatus("Extraction complete.");

                File.Delete(zipTargetPath);
                AppendStatus("Deleted ZIP after extraction.");

                string exeType = RadioIso.IsChecked == true ? "iso" : "chd";
                string arguments = $"/k cd /d \"{selectedFolder}\" && run_renamer.bat {exeType}";

                AppendStatus($"Launching renamer batch for: {exeType.ToUpper()}");

                var psi = new ProcessStartInfo
                {
                    FileName = "cmd.exe",
                    Arguments = arguments,
                    UseShellExecute = true,
                    WindowStyle = ProcessWindowStyle.Normal,
                    WorkingDirectory = selectedFolder,
                };

                Process.Start(psi);
                AppendStatus("Command prompt launched. Batch will handle cleanup.");
            }
            catch (Exception ex)
            {
                AppendStatus($"ERROR: {ex.Message}");
            }
            finally
            {
                BtnSelectFolder.IsEnabled = true;
                BtnRunRenamer.IsEnabled = true;
                UseEmbeddedZipCheckbox.IsEnabled = true;
                UpdateZipControls();
            }
        }

        private void UseEmbeddedZipCheckbox_Checked(object sender, RoutedEventArgs e)
        {
            UpdateZipControls();
        }

        private void UseEmbeddedZipCheckbox_Unchecked(object sender, RoutedEventArgs e)
        {
            UpdateZipControls();
        }

        private void UpdateZipControls()
        {
            // Defensive null checks
            if (UseEmbeddedZipCheckbox == null || ZipFilePathTextBox == null || BtnBrowseZip == null)
                return;

            bool useEmbedded = UseEmbeddedZipCheckbox.IsChecked == true;
            ZipFilePathTextBox.IsEnabled = !useEmbedded;
            BtnBrowseZip.IsEnabled = !useEmbedded;
        }

        private void AppendStatus(string message)
        {
            StatusText.Text += $"{DateTime.Now:HH:mm:ss} - {message}\n";
            StatusText.ScrollToEnd();
        }
    }
}
