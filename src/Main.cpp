#include <csignal>
#include <iostream>
#include <ranges>
#include <string>

#include <Penrose/Assets/AssetDictionary.hpp>
#include <Penrose/Assets/AssetManager.hpp>
#include <Penrose/Core/Engine.hpp>
#include <Penrose/ECS/ECSManager.hpp>
#include <Penrose/Rendering/RenderContext.hpp>
#include <Penrose/Scene/SceneManager.hpp>

#include <Penrose/Builtin/ECS/CameraComponent.hpp>
#include <Penrose/Builtin/ECS/MeshRendererComponent.hpp>
#include <Penrose/Builtin/ECS/RenderSourceComponent.hpp>
#include <Penrose/Builtin/ECS/TransformComponent.hpp>
#include <Penrose/Builtin/Rendering/ForwardSceneDrawRenderOperator.hpp>
#include <Penrose/Builtin/Rendering/ImGuiDrawRenderOperator.hpp>

class CameraRotateSystem : public Penrose::System {
public:
    explicit CameraRotateSystem(Penrose::ResourceSet *resources)
            : _ecsManager(resources->get<Penrose::ECSManager>()) {
        //
    }

    ~CameraRotateSystem() override = default;

    void update(float delta) override {
        auto cameras = this->_ecsManager->queryComponents<Penrose::CameraComponent>();

        for (const auto &camera: cameras) {
            auto transform = this->_ecsManager->tryGetComponent<Penrose::TransformComponent>(camera);

            if (!transform.has_value()) {
                continue;
            }

            transform->get()->getRot().y += glm::radians(delta * 45.0f);

            if (transform->get()->getRot().y > glm::radians(360.0f)) {
                transform->get()->getRot().y -= glm::radians(360.0f);
            } else if (transform->get()->getRot().y < glm::radians(0.0f)) {
                transform->get()->getRot().y += glm::radians(360.0f);
            }
        }
    }

    [[nodiscard]] constexpr static std::string_view name() { return "CameraRotate"; }

private:
    Penrose::ECSManager *_ecsManager;
};

void printError(const std::exception &error, int level) { // NOLINT(misc-no-recursion)
    if (level > 1) {
        std::cerr << std::string(2 * level, ' ');
    }

    if (level != 0) {
        std::cerr << "-> ";
    }

    std::cerr << error.what() << std::endl;

    try {
        std::rethrow_if_nested(error);
    } catch (const std::exception &nested) {
        printError(nested, level + 1);
    }
}

int main() {
    std::set_terminate([]() {
        auto caught = std::current_exception();

        if (caught) {
            std::cerr << "Unhandled engine exception" << std::endl;

            try {
                std::rethrow_exception(caught);
            } catch (const std::exception &error) {
                printError(error, 0);
            }
        } else {
            std::cerr << "Abnormal termination" << std::endl;
        }

#if defined(SIGTRAP)
        std::raise(SIGTRAP);
#endif

        std::abort();
    });

    Penrose::Engine engine;

    engine.resources().add<Penrose::RenderOperatorFactory, Penrose::ImGuiDrawRenderOperatorFactory>();

    auto assetDictionary = engine.resources().get<Penrose::AssetDictionary>();
    assetDictionary->addDir("data");

    auto assetManager = engine.resources().get<Penrose::AssetManager>();
    assetManager->queueMeshLoading("models/cube.obj");
    assetManager->queueImageLoading("textures/cube.png");
    assetManager->queueShaderLoading("shaders/default-forward-rendering.frag.spv");
    assetManager->queueShaderLoading("shaders/default-forward-rendering.vert.spv");

    auto graph = Penrose::RenderGraph()
            .setTarget("swapchain", Penrose::RenderTarget::makeSwapchain())
            .setTarget("depth", Penrose::RenderTarget::makeImage(Penrose::RenderTargetType::DepthStencil,
                                                                 Penrose::RenderFormat::D32Float,
                                                                 std::nullopt))
            .setSubgraph("default", Penrose::RenderSubgraph()
                    .addAttachment(Penrose::RenderAttachment("swapchain")
                                           .setClearValue(Penrose::RenderAttachmentClearValue({0, 0, 0, 1}))
                                           .setLoadOp(Penrose::RenderAttachmentLoadOp::Clear)
                                           .setStoreOp(Penrose::RenderAttachmentStoreOp::Store)
                                           .setInitialLayout(Penrose::RenderAttachmentLayout::Undefined)
                                           .setFinalLayout(Penrose::RenderAttachmentLayout::Present))
                    .addAttachment(Penrose::RenderAttachment("depth")
                                           .setFormat(Penrose::RenderFormat::D32Float)
                                           .setClearValue(Penrose::RenderAttachmentClearValue().setDepth(1))
                                           .setLoadOp(Penrose::RenderAttachmentLoadOp::Clear)
                                           .setStoreOp(Penrose::RenderAttachmentStoreOp::Store)
                                           .setInitialLayout(Penrose::RenderAttachmentLayout::Undefined)
                                           .setFinalLayout(Penrose::RenderAttachmentLayout::DepthStencilAttachment))
                    .addPass(Penrose::RenderPass()
                                     .addColorAttachmentIdx(0)
                                     .setDepthStencilAttachment(1)
                                     .setOperator(Penrose::RenderPassOperator(
                                             std::string(Penrose::ForwardSceneDrawRenderOperator::NAME))))
                    .addPass(Penrose::RenderPass()
                                     .addDependencyIdx(0)
                                     .addColorAttachmentIdx(0)
                                     .setOperator(Penrose::RenderPassOperator(
                                             std::string(Penrose::ImGuiDrawRenderOperator::NAME))))
            );

    auto renderContext = engine.resources().get<Penrose::RenderContext>();
    renderContext->setRenderGraph(graph);

    auto ecsManager = engine.resources().get<Penrose::ECSManager>();
    ecsManager->registerSystem<CameraRotateSystem>();

    auto sceneManager = engine.resources().get<Penrose::SceneManager>();
    auto root = sceneManager->addRoot("Default");

    for (int x: std::ranges::views::iota(-2, 3)) {
        auto entity = ecsManager->createEntity();

        auto meshRenderer = ecsManager->addComponent<Penrose::MeshRendererComponent>(entity);
        meshRenderer->setMeshAsset("models/cube.obj");
        meshRenderer->setAlbedoTextureAsset("textures/cube.png");

        auto transform = ecsManager->addComponent<Penrose::TransformComponent>(entity);
        transform->getPos() = glm::vec3(2.5 * x, 0, 4);

        sceneManager->insertEntityNode(root, entity);
    }

    {
        auto entity = ecsManager->createEntity();

        ecsManager->addComponent<Penrose::RenderSourceComponent>(entity);

        auto camera = ecsManager->addComponent<Penrose::CameraComponent>(entity);
        camera->getFov() = glm::radians(120.0f);

        auto transform = ecsManager->addComponent<Penrose::TransformComponent>(entity);
        transform->getPos() = glm::vec3(0, 0, -4);
        transform->getRot() = glm::vec3(0, glm::radians(-90.0f), 0);

        sceneManager->insertEntityNode(root, entity);
    }

    engine.run();

    return 0;
}
