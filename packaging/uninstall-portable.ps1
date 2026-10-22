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
        $label.Text = Text "请选择操作：`r`n`r`n卸载：移除任务，永久删除数据和整个便携目录，不经过回收站。`r`n仅卸载定时任务：保留程序和数据，方便移动目录。" "Choose an action:`r`n`r`nUninstall: Remove the task and permanently delete all data and the portable folder, without using the Recycle Bin.`r`nTask only: Keep the program and data so you can move the folder."
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
function Remove-DirectoryPermanently([string]$path) {
    # Explicit native filesystem deletion; never use the Shell or the Recycle Bin.
    Add-Type @'
using System;
using System.IO;
public static class BwlDelete {
    static bool GitName(string name) {
        return String.Equals(name.Split(':')[0].TrimEnd(' ', '.'), ".git", StringComparison.OrdinalIgnoreCase);
    }
    static void Guard(string path) {
        if (!Path.IsPathRooted(path) || (path.Length > 1 && path[1] == ':' && (path.Length < 3 || path[2] != '\\')))
            throw new IOException("An absolute application directory is required.");
        string full = Path.GetFullPath(path).TrimEnd('\\');
        if (String.Equals(full, Path.GetPathRoot(full).TrimEnd('\\'), StringComparison.OrdinalIgnoreCase))
            throw new IOException("Refusing to remove a filesystem root.");
        foreach (string part in full.Split('\\', '/')) if (GitName(part))
            throw new IOException("Refusing to remove Git metadata.");
        for (DirectoryInfo parent = Directory.GetParent(full); parent != null; parent = parent.Parent)
            if ((parent.Attributes & FileAttributes.ReparsePoint) != 0)
                throw new IOException("Refusing deletion through a parent junction or symbolic link.");
    }
    static void CheckTree(string path) {
        if (GitName(Path.GetFileName(path))) throw new IOException("Refusing to remove Git metadata.");
        FileAttributes attributes = File.GetAttributes(path);
        if ((attributes & FileAttributes.Directory) == 0 || (attributes & FileAttributes.ReparsePoint) != 0) return;
        foreach (string child in Directory.GetFileSystemEntries(path)) CheckTree(child);
    }
    static void DeleteTree(string path) {
        if (GitName(Path.GetFileName(path))) throw new IOException("Refusing to remove Git metadata.");
        FileAttributes attributes = File.GetAttributes(path);
        if ((attributes & FileAttributes.Directory) != 0) {
            if ((attributes & FileAttributes.ReparsePoint) == 0)
                foreach (string child in Directory.GetFileSystemEntries(path)) DeleteTree(child);
            Directory.Delete(path, false);
        } else File.Delete(path);
    }
    public static void RemoveDirectory(string path) {
        Guard(path);
        CheckTree(path); // Reject unsafe trees before deleting any file.
        DeleteTree(path);
        if (Directory.Exists(path) || File.Exists(path)) throw new IOException("Directory removal was incomplete.");
    }
}
'@
    [BwlDelete]::RemoveDirectory($path)
}
try {
    if ($Mode -eq 'Ask') {
        if ($Quiet) { throw (Text '自动化调用请显式指定 -Mode Uninstall 或 -Mode TasksOnly。' 'For unattended use, specify -Mode Uninstall or -Mode TasksOnly.') }
        $Mode = Select-Action
        if ($Mode -eq 'Cancel') { exit 0 }
    }
    $root = [IO.Path]::GetFullPath($AppDirectory).TrimEnd('\')
    if ($root -ne $PSScriptRoot.TrimEnd('\') -or $root -match '(?i)(?:^|[\\/])\.git[ .]*(?:[\\/]|$)' -or
        !(Test-Path -LiteralPath (Join-Path $root 'portable.tag')) -or
        !(Test-Path -LiteralPath (Join-Path $root 'BingWallpaperWorker.exe')) -or
        @(Get-ChildItem -LiteralPath $root -Filter '.git' -Force -Recurse -ErrorAction Stop).Count) {
        throw (Text '安全检查失败。请仅在完整解压的便携版目录中运行卸载脚本；目录中不能包含 .git。' 'Safety check failed. Run this script only inside a fully extracted portable folder containing no .git directory or file.')
    }
    # Do not delete a running settings executable. Never kill unrelated apps.
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
    Remove-DirectoryPermanently $root
    Show-Result (Text "卸载成功。`r`n`r`n定时任务已移除，用户数据和整个便携目录已永久删除，没有移入回收站。`r`n`r`n原目录：$root" "Uninstall completed successfully.`r`n`r`nThe scheduled task was removed. All user data and the portable folder were permanently deleted, not moved to the Recycle Bin.`r`n`r`nOriginal folder: $root") $true
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
