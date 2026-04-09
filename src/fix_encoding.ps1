# fix_encoding.ps1
# PowerShell 이중인코딩 복구 스크립트
# 사용: 프로젝트 루트에서 실행
#   powershell -ExecutionPolicy Bypass -File fix_encoding.ps1
 
Write-Host "=== 소스 파일 인코딩 복구 ===" -ForegroundColor Cyan
Write-Host ""
 
# git 로 원본 복원 (가장 확실한 방법)
Write-Host "[1/2] git 으로 이중인코딩 파일 복원 중..." -ForegroundColor Yellow
 
$filesToRestore = @(
    "src/EnhancedWatchdog.cpp",
    "src/ConfigManager.cpp"
)
 
foreach ($file in $filesToRestore) {
    if (Test-Path $file) {
        git checkout -- $file 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  ✅ 복원: $file"
        } else {
            Write-Host "  ⚠️  git 복원 실패: $file (git 이력이 없을 수 있음)"
        }
    }
}
 
Write-Host ""
Write-Host "[2/2] 나머지 .cpp .h 파일 UTF-8 재저장 중..." -ForegroundColor Yellow
Write-Host "  (이미 UTF-8 이면 변경 없음)"
 
$count = 0
Get-ChildItem -Recurse -Path "src","include" -Include "*.cpp","*.h","*.hpp" | ForEach-Object {
    try {
        # 현재 인코딩으로 읽기 (UTF-8 우선, 실패 시 CP949)
        $bytes = [System.IO.File]::ReadAllBytes($_.FullName)
        
        # UTF-8 BOM 확인 (EF BB BF)
        $hasBOM = ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF)
        
        if ($hasBOM) {
            # BOM 제거하고 UTF-8로 다시 저장
            $content = [System.Text.Encoding]::UTF8.GetString($bytes, 3, $bytes.Length - 3)
            [System.IO.File]::WriteAllText($_.FullName, $content, (New-Object System.Text.UTF8Encoding $false))
            Write-Host "  BOM 제거: $($_.Name)"
            $count++
        }
    } catch {
        Write-Host "  ⚠️  오류: $($_.Name): $_"
    }
}
 
Write-Host ""
Write-Host "=== 완료 ===" -ForegroundColor Green
Write-Host "처리된 파일: $count 개"
Write-Host ""
Write-Host "다음 단계:"
Write-Host "  1. VS Code에서 EnhancedWatchdog.cpp, ConfigManager.cpp 열기"
Write-Host "  2. 우하단 인코딩 표시가 'UTF-8' 인지 확인"
Write-Host "  3. pio run 실행"
 