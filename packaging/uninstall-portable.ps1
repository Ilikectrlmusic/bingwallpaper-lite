param([Parameter(Mandatory=$true)][string]$AppDirectory,
    [ValidateSet('Ask','Uninstall','TasksOnly')][string]$Mode = 'Ask',
    [ValidateSet('zh','en')][string]$Language = 'zh', [switch]$Quiet)
$ErrorActionPreference = 'Stop'
$cleanupDone = $false
function Text([string]$zh, [string]$en) {
    if ($Language -eq 'zh') { return $zh }
    return $en
}
[Threading.Thread]::CurrentThread.CurrentUICulture = [Globalization.CultureInfo]::GetCultureInfo((Text 'zh-CN' 'en-US'))
function Select-Action {
    Add-Type -AssemblyName System.Windows.Forms
    Add-Type -AssemblyName System.Drawing
    $form = New-Object Windows.Forms.Form
    try {
        $form.Text = Text 'Bing Wallpaper - 便携版卸载' 'Bing Wallpaper - Portable uninstall'
        $form.ClientSize = New-Object Drawing.Size(520, 204)
        $form.StartPosition = 'CenterScreen'
        $form.FormBorderStyle = 'FixedDialog'
        $form.MaximizeBox = $false; $form.MinimizeBox = $false
        $form.Font = New-Object Drawing.Font((Text 'Microsoft YaHei UI' 'Segoe UI'), 9)
        $label = New-Object Windows.Forms.Label
        $label.Text = Text "请选择操作：`r`n`r`n卸载：清理任务和数据，将整个便携目录移到回收站。`r`n仅卸载定时任务：保留程序和数据，方便移动目录。" "Choose an action:`r`n`r`nUninstall: Remove the task and data; move the whole folder to the Recycle Bin.`r`nTask only: Keep the program and data so you can move the folder."
        $label.SetBounds(20, 16, 480, 125)
        $form.Controls.Add($label)
        $buttons = @(@((Text '卸载' 'Uninstall'), 'Yes', 20, 100), @((Text '仅卸载定时任务' 'Remove scheduled task only'), 'No', 132, 230), @((Text '取消' 'Cancel'), 'Cancel', 374, 126))
        foreach ($item in $buttons) {
            $button = New-Object Windows.Forms.Button
            $button.Text = $item[0]; $button.DialogResult = $item[1]
            $button.SetBounds($item[2], 158, $item[3], 30)
            $form.Controls.Add($button)
            if ($item[1] -eq 'Cancel') { $form.CancelButton = $button; $form.AcceptButton = $button }
        }
        $answer = $form.ShowDialog()
        if ($answer -eq [Windows.Forms.DialogResult]::Yes) { return 'Uninstall' }
        if ($answer -eq [Windows.Forms.DialogResult]::No) { return 'TasksOnly' }
        return 'Cancel'
    } finally { $form.Dispose() }
}
function Show-Result([string]$message, [bool]$success) {
    Write-Host $message
    if (!$Quiet) {
        Add-Type -AssemblyName System.Windows.Forms
        $icon = if ($success) { [Windows.Forms.MessageBoxIcon]::Information } else { [Windows.Forms.MessageBoxIcon]::Error }
        [void][Windows.Forms.MessageBox]::Show($message, (Text 'Bing Wallpaper - 卸载结果' 'Bing Wallpaper - Uninstall result'), [Windows.Forms.MessageBoxButtons]::OK, $icon)
    }
}
function Move-DirectoryToRecycleBin([string]$path) {
    # Force recycling, never allow a permanent-delete fallback or hidden native error prompts.
    Add-Type @'
using System;
using System.IO;
using System.Runtime.InteropServices;
[ComImport, Guid("43826D1E-E718-42EE-BC55-A1E261C37BFE"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface IBwlShellItem {}
[ComImport, Guid("947AAB5F-0A5C-4C13-B4D6-4BF7836FC9F8"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface IBwlFileOperation {
    [PreserveSig] int Advise(IntPtr sink, out uint cookie);
    [PreserveSig] int Unadvise(uint cookie);
    [PreserveSig] int SetOperationFlags(uint flags);
    [PreserveSig] int SetProgressMessage([MarshalAs(UnmanagedType.LPWStr)] string message);
    [PreserveSig] int SetProgressDialog(IntPtr dialog);
    [PreserveSig] int SetProperties(IntPtr properties);
    [PreserveSig] int SetOwnerWindow(IntPtr owner);
    [PreserveSig] int ApplyPropertiesToItem(IBwlShellItem item);
    [PreserveSig] int ApplyPropertiesToItems(IntPtr items);
    [PreserveSig] int RenameItem(IBwlShellItem item, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr sink);
    [PreserveSig] int RenameItems(IntPtr items, [MarshalAs(UnmanagedType.LPWStr)] string name);
    [PreserveSig] int MoveItem(IBwlShellItem item, IBwlShellItem destination, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr sink);
    [PreserveSig] int MoveItems(IntPtr items, IBwlShellItem destination);
    [PreserveSig] int CopyItem(IBwlShellItem item, IBwlShellItem destination, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr sink);
    [PreserveSig] int CopyItems(IntPtr items, IBwlShellItem destination);
    [PreserveSig] int DeleteItem(IBwlShellItem item, IntPtr sink);
    [PreserveSig] int DeleteItems(IntPtr items);
    [PreserveSig] int NewItem(IBwlShellItem destination, uint attributes, [MarshalAs(UnmanagedType.LPWStr)] string name, [MarshalAs(UnmanagedType.LPWStr)] string template, IntPtr sink);
    [PreserveSig] int PerformOperations();
    [PreserveSig] int GetAnyOperationsAborted([MarshalAs(UnmanagedType.Bool)] out bool aborted);
}
public static class BwlRecycle {
    [DllImport("shell32.dll", CharSet=CharSet.Unicode)]
    static extern int SHCreateItemFromParsingName(string path, IntPtr context, ref Guid iid, [MarshalAs(UnmanagedType.Interface)] out IBwlShellItem item);
    public static void MoveDirectory(string path, string errorMessage) {
        IBwlFileOperation operation = null; IBwlShellItem item = null;
        try {
            operation = (IBwlFileOperation)Activator.CreateInstance(Type.GetTypeFromCLSID(new Guid("3AD05575-8857-4850-9277-11B85BDB8E09")));
            Marshal.ThrowExceptionForHR(operation.SetOperationFlags(0x00180454));
            Guid iid = typeof(IBwlShellItem).GUID;
            Marshal.ThrowExceptionForHR(SHCreateItemFromParsingName(path, IntPtr.Zero, ref iid, out item));
            Marshal.ThrowExceptionForHR(operation.DeleteItem(item, IntPtr.Zero));
            Marshal.ThrowExceptionForHR(operation.PerformOperations());
            bool aborted;
            Marshal.ThrowExceptionForHR(operation.GetAnyOperationsAborted(out aborted));
            if (aborted || Directory.Exists(path)) throw new IOException(errorMessage);
        } finally {
            if (item != null) Marshal.FinalReleaseComObject(item);
            if (operation != null) Marshal.FinalReleaseComObject(operation);
        }
    }
}
'@
    [BwlRecycle]::MoveDirectory($path, (Text '回收站操作未能移除程序目录。' 'The Recycle Bin operation did not remove the program directory.'))
}
try {
    if ($Mode -eq 'Ask') {
        if ($Quiet) { throw (Text '自动化调用请显式指定 -Mode Uninstall 或 -Mode TasksOnly。' 'For unattended use, specify -Mode Uninstall or -Mode TasksOnly.') }
        $Mode = Select-Action
        if ($Mode -eq 'Cancel') { exit 0 }
    }
    $root = [IO.Path]::GetFullPath($AppDirectory).TrimEnd('\')
    if ($root -ne $PSScriptRoot.TrimEnd('\') -or
        !(Test-Path -LiteralPath (Join-Path $root 'portable.tag')) -or
        !(Test-Path -LiteralPath (Join-Path $root 'BingWallpaperWorker.exe')) -or
        @(Get-ChildItem -LiteralPath $root -Filter '.git' -Force -Recurse -ErrorAction Stop).Count) {
        throw (Text '安全检查失败。请仅在完整解压的便携版目录中运行卸载脚本；目录中不能包含 .git。' 'Safety check failed. Run this script only inside a fully extracted portable folder containing no .git directory or file.')
    }
    # Do not move a running settings executable. Let the user close it, never kill unrelated apps.
    $gui = @(Get-Process -Name BingWallpaper,BingWallpaperLite -ErrorAction SilentlyContinue | Where-Object {
        try { $_.Path -in @((Join-Path $root 'BingWallpaper.exe'), (Join-Path $root 'BingWallpaperLite.exe')) } catch { $false }
    })
    if ($gui.Count) { throw (Text '请先关闭 Bing Wallpaper 设置窗口，再重新运行卸载。' 'Close the Bing Wallpaper settings window, then run uninstall again.') }
    $workerMode = if ($Mode -eq 'TasksOnly') { '--remove-task' } else { '--uninstall' }
    $process = Start-Process -FilePath (Join-Path $root 'BingWallpaperWorker.exe') -ArgumentList $workerMode -WindowStyle Hidden -PassThru -Wait
    if ($process.ExitCode -ne 0) { throw (Text "工作程序清理失败（错误码 $($process.ExitCode)）。请关闭设置窗口，并使用设置此软件的同一 Windows 用户重试。详细原因请查看 data/logs 或 APPDATA 中的日志。" "Worker cleanup failed (exit code $($process.ExitCode)). Close settings and retry using the Windows account that configured this app. See logs in data/logs or APPDATA for details.") }
    if ($Mode -eq 'TasksOnly') {
        Show-Result (Text "定时任务卸载成功。`r`n`r`n程序、设置、缓存和日志均已保留。现在可以移动整个便携目录；移动后运行 BingWallpaper.exe，即可为新位置重新建立定时任务。`r`n`r`n目录：$root" "Scheduled task removed successfully.`r`n`r`nThe program, settings, cache and logs are preserved. You can move the whole folder. Run BingWallpaper.exe at the new location to register the daily task.`r`n`r`nFolder: $root") $true
        exit 0
    }
    $cleanupDone = $true
    Set-Location -LiteralPath $env:TEMP
    Move-DirectoryToRecycleBin $root
    Show-Result (Text "卸载成功。`r`n`r`n定时任务和用户数据已清理，整个便携目录已移到回收站，无需手动删除。`r`n`r`n原目录：$root" "Uninstall completed successfully.`r`n`r`nThe scheduled task and user data were removed. The whole portable folder was moved to the Recycle Bin; no manual deletion is needed.`r`n`r`nOriginal folder: $root") $true
    exit 0
} catch {
    $action = if ($Mode -eq 'TasksOnly') { Text '定时任务卸载失败' 'Scheduled task removal failed' } else { Text '卸载失败' 'Uninstall failed' }
    $message = "${action}: $($_.Exception.Message)"
    if ($cleanupDone) {
        $message += Text "`r`n`r`n定时任务和用户数据已清理，但程序目录未能自动删除。请关闭占用文件的程序后，手动删除：`r`n$root" "`r`n`r`nThe task and user data were removed, but the program folder could not be removed. Close any apps using these files, then remove this folder manually:`r`n$root"
    } else {
        $message += Text "`r`n`r`n程序目录已保留。请修复上述错误后重试，不要只删除目录，以免留下定时任务。`r`n目录：$AppDirectory" "`r`n`r`nThe program folder was preserved. Resolve the error and retry. Do not just delete the folder, as its scheduled task may remain.`r`nFolder: $AppDirectory"
    }
    Show-Result $message $false
    exit 1
}
