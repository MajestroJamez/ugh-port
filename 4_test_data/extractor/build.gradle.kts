plugins {
    alias(libs.plugins.kotlin.jvm)
    alias(libs.plugins.kotlin.serialization)
    application
}

kotlin {
    jvmToolchain(25)
}

dependencies {
    implementation(project(":core"))
    implementation(libs.serialization.json)
    testImplementation(platform(libs.junit.bom))
    testImplementation(libs.junit.jupiter)
    testRuntimeOnly(libs.junit.launcher)
}

application {
    mainClass = "ugh.extractor.MainKt"
}

val originalExe = rootProject.layout.projectDirectory.file("1_original/UGH.EXE")
val assetsDir = rootProject.layout.projectDirectory.dir("assets")

tasks.named<JavaExec>("run") {
    args(originalExe.asFile.absolutePath, assetsDir.asFile.absolutePath)
}

// The original's sounds and music as WAV files for the remake (Sounds.kt), into assets/sound.
tasks.register<JavaExec>("sound") {
    description = "Renders the original's sounds and music with the port's OPL2 synthesizer into assets/sound."
    group = "application"
    classpath = sourceSets.main.get().runtimeClasspath
    mainClass = "ugh.extractor.SoundsKt"
    args(originalExe.asFile.absolutePath, assetsDir.dir("sound").asFile.absolutePath)
}

tasks.test {
    useJUnitPlatform()
    systemProperty("ugh.exe", originalExe.asFile.absolutePath)
}
