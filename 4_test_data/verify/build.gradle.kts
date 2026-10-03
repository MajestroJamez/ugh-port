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

tasks.withType<Test>().configureEach {
    useJUnitPlatform()
    systemProperty("ugh.exe", rootProject.layout.projectDirectory.file("1_original/UGH.EXE").asFile.absolutePath)
    systemProperty("ugh.out", layout.buildDirectory.dir("verify-out").get().asFile.absolutePath)
    systemProperty("ugh.replays", layout.buildDirectory.dir("replays").get().asFile.absolutePath)
    if (project.hasProperty("ugh.explore")) systemProperty("ugh.explore", "1")
    maxHeapSize = "6g"
    testLogging { showStandardStreams = true }
}

// Golden replays ("UGR 1") for the C++ logic, into build/replays; the full test run records them too.
tasks.register<Test>("replays") {
    description = "Records the golden replays in lockstep with the original into build/replays."
    group = "verification"
    testClassesDirs = sourceSets.test.get().output.classesDirs
    classpath = sourceSets.test.get().runtimeClasspath
    filter { includeTestsMatching("ugh.verify.replay.*") }
    outputs.upToDateWhen { false }
}
