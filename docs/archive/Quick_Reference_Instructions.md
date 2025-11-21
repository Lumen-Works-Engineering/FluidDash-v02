# Quick Reference: Using the Claude Code Instructions

## Document Created
**Location**: `C:\Users\john_sparks\Documents\PlatformIO\Projects\FluidDash-v02\Claude_Code_Instructions_WebSocket_Extraction.md`

## What This Does
This instruction document guides Claude Code (VS Code extension) through the final refactoring step: extracting WebSocket handling logic from main.cpp into the network module.

## How to Use This Document

### Option 1: In VS Code with Claude Code Extension (RECOMMENDED)

1. **Open VS Code** in your FluidDash-v02 project
2. **Open Claude Code** (Ctrl+Shift+P → "Claude Code: Open")
3. **Upload the instruction document** to the Claude Code chat
4. **Say**: "Please follow these instructions to complete the WebSocket extraction refactoring"
5. Claude Code will:
   - Read the instructions
   - Make the changes to network.h, network.cpp, and main.cpp
   - Verify compilation
   - Provide a summary of changes

### Option 2: In Browser-Based Claude Code

1. Open Claude.ai/code
2. Connect to Desktop Commander MCP server
3. Upload the instruction document
4. Say: "Please follow these instructions to refactor the FluidDash project at `C:\Users\john_sparks\Documents\PlatformIO\Projects\FluidDash-v02`"

## What Will Be Changed

### Files Modified
1. **src/network/network.h** - Add `handleWebSocketLoop()` declaration
2. **src/network/network.cpp** - Add `handleWebSocketLoop()` implementation (~35 lines)
3. **src/main.cpp** - Replace ~25 lines of WebSocket code with single function call

### Expected Results
- main.cpp: 285 lines → ~260 lines
- All WebSocket logic consolidated in network module
- Cleaner, more maintainable code structure

## Benefits of This Refactoring

✅ **Better Organization**: All network operations in one place  
✅ **Improved Readability**: main.cpp loop() easier to understand  
✅ **Easier Testing**: Network module can be tested independently  
✅ **Future-Proof**: Easy to add new network features without touching main.cpp  

## Verification Steps

After Claude Code completes the task:

1. **Compile the project**:
   ```bash
   pio run
   ```

2. **Check the changes**:
   - Open main.cpp and look at the loop() function
   - Verify it now has a single `handleWebSocketLoop();` call
   - Open network.cpp and verify the new function exists

3. **Test on hardware** (if available):
   - Upload to ESP32
   - Verify WebSocket connection works
   - Check that FluidNC status updates properly

## What's Already Done

Your FluidDash project has already completed ALL major refactoring recommendations:

✅ Web handlers extracted to dedicated module (813 lines)  
✅ Global state organized into proper structs  
✅ main.cpp reduced from 1,192 → 285 lines  
✅ Button handling properly implemented  
✅ Timing variables organized  
✅ Storage manager properly accessible  

This WebSocket extraction is the **final polish** to achieve 100% modular architecture!

## Rollback (If Needed)

If anything goes wrong, the instruction document includes a rollback plan. The changes are isolated and easily reversible.

## Support

If you encounter any issues:
1. Check the "Testing Checklist" in the main instruction document
2. Review the "Expected Results" section
3. Use the "Rollback Plan" if needed
4. The original code is safe - Claude Code tracks all changes

---

**Ready to proceed?** Just open the instruction document in Claude Code and ask it to follow the steps!
