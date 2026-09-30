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

val originalExe = rootProject.layout.projectDirectory.file("OLD/UGH.EXE")
val assetsDir = rootProject.layout.projectDirectory.dir("assets")

tasks.named<JavaExec>("run") {
    args(originalExe.asFile.absolutePath, assetsDir.asFile.absolutePath)
}

tasks.test {
    useJUnitPlatform()
    systemProperty("ugh.exe", originalExe.asFile.absolutePath)
}
