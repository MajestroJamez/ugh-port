plugins {
    alias(libs.plugins.kotlin.jvm)
}

kotlin {
    jvmToolchain(25)
}

// Differential tests: the ported core against the original UGH.EXE running in the oracle.
dependencies {
    testImplementation(project(":core"))
    testImplementation(project(":oracle"))
    testImplementation(project(":extractor"))
    testImplementation(platform(libs.junit.bom))
    testImplementation(libs.junit.jupiter)
    testRuntimeOnly(libs.junit.launcher)
}

tasks.test {
    useJUnitPlatform()
    systemProperty("ugh.exe", rootProject.layout.projectDirectory.file("OLD/UGH.EXE").asFile.absolutePath)
    systemProperty("ugh.out", layout.buildDirectory.dir("verify-out").get().asFile.absolutePath)
    if (project.hasProperty("ugh.explore")) systemProperty("ugh.explore", "1")
    maxHeapSize = "2g"
    testLogging { showStandardStreams = true }
}
