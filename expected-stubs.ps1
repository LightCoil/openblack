# Script commands that are accepted by the script layer but have no gameplay
# effect yet. Land1 legitimately calls some of them, so verify.ps1 reports them
# instead of failing outright.
#
# Each entry is:
#   <FunctionName> = <expected hit count for Land1, or 'unused'>
#
# Rules enforced by verify.ps1:
#   - a stub hit that is NOT listed here fails the run (someone added a new stub)
#   - a listed stub whose count changed fails the run (regression or new work)
#   - reducing a count is a failure too: it usually means the log stopped being
#     written, which previously made this check meaningless
#
# To make progress: implement the command, then lower its count here. Do not
# silence a stub by commenting out its log call; LogUnimplementedScriptCommand in
# FeatureScriptCommands.cpp is the only sanctioned way to report one.

$script:ExpectedScriptStubs = @{
    'CreateNewAnimal'             = 116
    'CreateDrinkWaypoint'         = 47
    'CreateFlock'                 = 16
    'CreateTownFishFarm'          = 13
    'CreateWeatherClimate'        = 4
    'CreateWeatherClimateRain'    = 4
    'CreateWeatherClimateTemp'    = 4
    'CreateWeatherClimateWind'    = 4
    'CreateArena'                 = 3
}

# Stub commands that exist in FeatureScriptCommands.cpp but that Land1 never
# calls. They are listed so that a new call site cannot slip through unnoticed.
$script:KnownUnusedScriptStubs = @(
    'CreateCreaturePen'
    'CreateWorshipSite'
    'CreatePlannedWorshipSite'
    'CreateAnimal'
    'CreateFishFarm'
    'CreateWallSection'
    'CreatePlannedWallSection'
    'CreatePitch'
    'CreateTownTemporaryPots'
    'CreateScaffold'
    'CountryChange'
    'HeightChange'
    'CreateArea'
    'FlyByFile'
    'CreateFurniture'
    'CreateInfluenceRing'
    'CreateWeatherStorm'
    'BrushSize'
    'CreateWaterfall'
    'CreateBase'
    'MultiplayerDebug'
    'CreateOneShotSpell'
    'CreateOneShotSpellPu'
    'CreateFireFly'
    'CreateSpellDispenser'
    'LoadComputerPlayerPersonality'
    'EditLevel'
    'CreatePath'
    'CreateTownCentreSpellIcon'
    'CreateSpellIcon'
    'CreatePlannedSpellIcon'
    'CreateSpecialTownVillager'
    'CreateCreature'
    'CreateFlowers'
    'LinkFootpath'
)
