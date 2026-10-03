plugins {
    alias(libs.plugins.kotlin.jvm)
    application
}

kotlin {
    jvmToolchain(25)
}

dependencies {
    implementation(project(":core"))
    implementation(project(":oracle"))
}

application {
    mainClass = "ugh.desktop.MainKt"
    applicationDefaultJvmArgs = listOf("-Dsun.java2d.uiScale=1")
}

tasks.named<JavaExec>("run") {
    workingDir = rootProject.file("1_original") // GameFile looks for UGH.EXE in the working directory
}

// ---------------------------------------------------------------- Windows package
// UGH-port.exe with its own minimal Java runtime (jpackage app image, no installer needed) and a zip of it.
// The original UGH.EXE is not part of it: the player puts it next to UGH-port.exe or picks it on the first start.

val packageName = "UGH-port"
val jpackageDir = layout.buildDirectory.dir("jpackage")

val appImage = tasks.register<Exec>("appImage") {
    group = "distribution"
    description = "Builds build/jpackage/$packageName: $packageName.exe with a bundled Java runtime."
    dependsOn(tasks.named("installDist"))
    val launcher = javaToolchains.launcherFor { languageVersion = JavaLanguageVersion.of(25) }
    val lib = layout.buildDirectory.dir("install/desktop/lib")
    inputs.dir(lib)
    outputs.dir(jpackageDir)
    doFirst {
        delete(jpackageDir)
        executable = launcher.get().metadata.installationPath.file("bin/jpackage.exe").asFile.absolutePath
        args(
            "--type", "app-image",
            "--name", packageName,
            "--app-version", "1.0.0",
            "--description", "UGH! - a faithful port of the 1994 DOS game (needs the original UGH.EXE)",
            "--input", lib.get().asFile.absolutePath,
            "--main-jar", "desktop.jar",
            "--main-class", "ugh.desktop.MainKt",
            "--java-options", "-Dsun.java2d.uiScale=1",
            "--add-modules", "java.desktop",
            "--jlink-options", "--strip-debug --no-man-pages --no-header-files",
            "--dest", jpackageDir.get().asFile.absolutePath,
        )
    }
}

tasks.register<Zip>("packageZip") {
    group = "distribution"
    description = "Zips the Windows app image into build/distributions/$packageName-windows.zip."
    dependsOn(appImage)
    archiveFileName = "$packageName-windows.zip"
    destinationDirectory = layout.buildDirectory.dir("distributions")
    from(jpackageDir.map { it.dir(packageName) }) { into(packageName) }
    from(rootProject.layout.projectDirectory.file("3_kotlin_port/desktop/package/README.txt")) { into(packageName) }
}
