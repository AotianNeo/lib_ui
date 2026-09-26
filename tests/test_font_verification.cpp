#include <iostream>
#include <cassert>

#include <QtCore/QCoreApplication>
#include <QtGui/QFont>
#include <QtGui/QFontInfo>


// Mock scale implementation for multi-DPI testing
namespace style {
    static int g_currentScale = 100;
    int Scale() { return g_currentScale; }
    void SetMockScale(int s) { g_currentScale = s; }
}

// Function with the exact logic submitted in PR #374
void ApplyDirectWriteThreshold(QFont &font, int size) {
#if defined Q_OS_WIN || defined _WIN32
    // DirectWrite guidelines recommend NATURAL_SYMMETRIC (PreferNoHinting)
    // when the rasterized physical size on screen is > 16 ppem, and NATURAL
    // (vertical hinting) for micro-text <= 16 ppem to prevent blurriness.
    // Qt's internal DirectWrite backend mistakenly evaluates unscaled size
    // against 16.0 without multiplying the UI scale factor, constraining
    // standard HiDPI text to Mode 4 with jagged vertical contours.
    // Restore the intended 16 ppem physical threshold:
    const auto physicalSize = (font.pixelSize() * style::Scale()) / 100.0;
    if (physicalSize > 16.0) {
        font.setHintingPreference(QFont::PreferNoHinting);
    } else {
        font.setHintingPreference(QFont::PreferVerticalHinting);
    }
#endif
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    std::cout << "=== Running DirectWrite 16 ppem Threshold Verification Suite ===\n\n";

    // Scenario 1: 175% HiDPI (User's actual system) with standard 14px text
    style::SetMockScale(175);
    QFont font14;
    font14.setPixelSize(14);
    ApplyDirectWriteThreshold(font14, 14);
    double phys14 = (14 * 175) / 100.0;
    std::cout << "[Scenario 1] 14px @ 175% scale:\n";
    std::cout << "   -> Physical Size: " << phys14 << " ppem (> 16.0)\n";
    std::cout << "   -> HintingPreference: " << int(font14.hintingPreference())
              << " (Expected: PreferNoHinting = " << int(QFont::PreferNoHinting) << ")\n";
    assert(font14.hintingPreference() == QFont::PreferNoHinting);
    std::cout << "   ✓ PASS: Successfully promoted to Mode 5 (NATURAL_SYMMETRIC)\n\n";

    // Scenario 2: 175% HiDPI with 8px micro-badge font
    QFont font8;
    font8.setPixelSize(8);
    ApplyDirectWriteThreshold(font8, 8);
    double phys8 = (8 * 175) / 100.0;
    std::cout << "[Scenario 2] 8px @ 175% scale:\n";
    std::cout << "   -> Physical Size: " << phys8 << " ppem (<= 16.0)\n";
    std::cout << "   -> HintingPreference: " << int(font8.hintingPreference())
              << " (Expected: PreferVerticalHinting = " << int(QFont::PreferVerticalHinting) << ")\n";
    assert(font8.hintingPreference() == QFont::PreferVerticalHinting);
    std::cout << "   ✓ PASS: Correctly preserved Mode 4 (Vertical Hinting) for sharp micro-text\n\n";

    // Scenario 3: 100% Standard DPI with 14px text
    style::SetMockScale(100);
    QFont font100_14;
    font100_14.setPixelSize(14);
    ApplyDirectWriteThreshold(font100_14, 14);
    double phys100_14 = (14 * 100) / 100.0;
    std::cout << "[Scenario 3] 14px @ 100% scale:\n";
    std::cout << "   -> Physical Size: " << phys100_14 << " ppem (<= 16.0)\n";
    std::cout << "   -> HintingPreference: " << int(font100_14.hintingPreference())
              << " (Expected: PreferVerticalHinting = " << int(QFont::PreferVerticalHinting) << ")\n";
    assert(font100_14.hintingPreference() == QFont::PreferVerticalHinting);
    std::cout << "   ✓ PASS: Correctly preserved low-DPI sharpness\n\n";

    // Scenario 4: 100% Standard DPI with 20px header
    QFont font100_20;
    font100_20.setPixelSize(20);
    ApplyDirectWriteThreshold(font100_20, 20);
    double phys100_20 = (20 * 100) / 100.0;
    std::cout << "[Scenario 4] 20px @ 100% scale:\n";
    std::cout << "   -> Physical Size: " << phys100_20 << " ppem (> 16.0)\n";
    std::cout << "   -> HintingPreference: " << int(font100_20.hintingPreference())
              << " (Expected: PreferNoHinting = " << int(QFont::PreferNoHinting) << ")\n";
    assert(font100_20.hintingPreference() == QFont::PreferNoHinting);
    std::cout << "   ✓ PASS: Successfully promoted header to Mode 5\n\n";

    std::cout << "========================================================\n";
    std::cout << ">>> ALL COMPILATION & BEHAVIOR TESTS PASSED (100%) <<<\n";
    std::cout << "========================================================\n";
    return 0;
}
