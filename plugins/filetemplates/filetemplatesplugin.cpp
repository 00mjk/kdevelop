#include "filetemplatesplugin.h"
#include "templateclassassistant.h"
#include "templatepreviewtoolview.h"
#include "debug.h"

#include <language/codegen/templatesmodel.h>
#include <language/interfaces/editorcontext.h>
#include <interfaces/icore.h>
#include <interfaces/iproject.h>
#include <interfaces/iuicontroller.h>
#include <interfaces/context.h>
#include <interfaces/contextmenuextension.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/iselectioncontroller.h>
#include <interfaces/iprojectcontroller.h>
#include <project/projectmodel.h>
#include <util/path.h>

#include <KActionCollection>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QIcon>

using namespace KDevelop;

K_PLUGIN_FACTORY_WITH_JSON(FileTemplatesFactory, "kdevfiletemplates.json", registerPlugin<FileTemplatesPlugin>();)

class TemplatePreviewFactory : public KDevelop::IToolViewFactory
{
public:
    TemplatePreviewFactory(FileTemplatesPlugin* plugin)
    : KDevelop::IToolViewFactory()
    , m_plugin(plugin)
    {

    }

    QWidget* create(QWidget* parent = 0) override
    {
        return new TemplatePreviewToolView(m_plugin, parent);
    }

    QString id() const override
    {
        return QStringLiteral("org.kdevelop.TemplateFilePreview");
    }

    Qt::DockWidgetArea defaultPosition() override
    {
        return Qt::RightDockWidgetArea;
    }

private:
    FileTemplatesPlugin* m_plugin;
};

FileTemplatesPlugin::FileTemplatesPlugin(QObject* parent, const QVariantList& args)
    : IPlugin(QStringLiteral("kdevfiletemplates"), parent)
    , m_model(0)
{
    Q_UNUSED(args);
    KDEV_USE_EXTENSION_INTERFACE(ITemplateProvider)

    setXMLFile(QStringLiteral("kdevfiletemplates.rc"));
    QAction* action = actionCollection()->addAction(QStringLiteral("new_from_template"));
    action->setText( i18n( "New From Template" ) );
    action->setIcon( QIcon::fromTheme( QStringLiteral("code-class") ) );
    action->setWhatsThis( i18n( "Allows you to create new source code files, such as classes or unit tests, using templates." ) );
    action->setStatusTip( i18n( "Create new files from a template" ) );
    connect (action, &QAction::triggered, this, &FileTemplatesPlugin::createFromTemplate);

    m_toolView = new TemplatePreviewFactory(this);
    core()->uiController()->addToolView(i18n("Template Preview"), m_toolView);
}

FileTemplatesPlugin::~FileTemplatesPlugin()
{

}

void FileTemplatesPlugin::unload()
{
    core()->uiController()->removeToolView(m_toolView);
}

ContextMenuExtension FileTemplatesPlugin::contextMenuExtension (Context* context)
{
    ContextMenuExtension ext;
    QUrl fileUrl;

    if (context->type() == Context::ProjectItemContext)
    {
        ProjectItemContext* projectContext = dynamic_cast<ProjectItemContext*>(context);
        QList<ProjectBaseItem*> items = projectContext->items();
        if (items.size() != 1)
        {
            return ext;
        }

        QUrl url;
        ProjectBaseItem* item = items.first();
        if (item->folder())
        {
            url = item->path().toUrl();
        }
        else if (item->target())
        {
            url = item->parent()->path().toUrl();
        }
        if (url.isValid())
        {
            QAction* action = new QAction(i18n("Create From Template"), this);
            action->setIcon(QIcon::fromTheme(QStringLiteral("code-class")));
            action->setData(url);
            connect(action, &QAction::triggered, this, &FileTemplatesPlugin::createFromTemplate);
            ext.addAction(ContextMenuExtension::FileGroup, action);
        }

        if (item->file())
        {
            fileUrl = item->path().toUrl();
        }
    }
    else if (context->type() == Context::EditorContext)
    {
        KDevelop::EditorContext* editorContext = dynamic_cast<KDevelop::EditorContext*>(context);
        fileUrl = editorContext->url();
    }

    if (fileUrl.isValid() && determineTemplateType(fileUrl) != NoTemplate)
    {
        QAction* action = new QAction(i18n("Show Template Preview"), this);
        action->setIcon(QIcon::fromTheme(QStringLiteral("document-preview")));
        action->setData(fileUrl);
        connect(action, &QAction::triggered, this, &FileTemplatesPlugin::previewTemplate);
        ext.addAction(ContextMenuExtension::ExtensionGroup, action);
    }

    return ext;
}

QString FileTemplatesPlugin::name() const
{
    return i18n("File Templates");
}

QIcon FileTemplatesPlugin::icon() const
{
    return QIcon::fromTheme(QStringLiteral("code-class"));
}

QAbstractItemModel* FileTemplatesPlugin::templatesModel()
{
    if(!m_model) {
        m_model = new TemplatesModel(QStringLiteral("kdevfiletemplates"), this);
    }
    return m_model;
}

QString FileTemplatesPlugin::knsConfigurationFile() const
{
    return QStringLiteral("kdevfiletemplates.knsrc");
}

QStringList FileTemplatesPlugin::supportedMimeTypes() const
{
    QStringList types;
    types << QStringLiteral("application/x-desktop");
    types << QStringLiteral("application/x-bzip-compressed-tar");
    types << QStringLiteral("application/zip");
    return types;
}

void FileTemplatesPlugin::reload()
{
    templatesModel();
    m_model->refresh();
}

void FileTemplatesPlugin::loadTemplate(const QString& fileName)
{
    templatesModel();
    m_model->loadTemplateFile(fileName);
}

void FileTemplatesPlugin::createFromTemplate()
{
    QUrl baseUrl;
    if (QAction* action = qobject_cast<QAction*>(sender()))
    {
        baseUrl = action->data().toUrl();
    }
    if (!baseUrl.isValid()) {
        // fall-back to currently active document's parent directory
        IDocument* doc = ICore::self()->documentController()->activeDocument();
        if (doc && doc->url().isValid()) {
            baseUrl = doc->url().adjusted(QUrl::RemoveFilename);
        }
    }
    if (!baseUrl.isValid()) {
        // fall-back to currently selected project's or item's base directory
        ProjectItemContext* projectContext = dynamic_cast<ProjectItemContext*>(ICore::self()->selectionController()->currentSelection());
        if (projectContext) {
            const QList<ProjectBaseItem*> items = projectContext->items();
            if (items.size() == 1) {
                ProjectBaseItem* item = items.at(0);
                if (item->folder()) {
                    baseUrl = item->path().toUrl();
                } else if (item->target()) {
                    baseUrl = item->parent()->path().toUrl();
                }
            }
        }
    }
    if (!baseUrl.isValid()) {
        // fall back to base directory of currently open project, if there is only one
        const QList<IProject*> projects = ICore::self()->projectController()->projects();
        if (projects.size() == 1) {
            baseUrl = projects.at(0)->path().toUrl();
        }
    }
    if (!baseUrl.isValid()) {
        // last resort: home path
        baseUrl = QUrl::fromLocalFile(QDir::homePath());
    }
    TemplateClassAssistant* assistant = new TemplateClassAssistant(QApplication::activeWindow(), baseUrl);
    assistant->setAttribute(Qt::WA_DeleteOnClose);
    assistant->show();
}

FileTemplatesPlugin::TemplateType FileTemplatesPlugin::determineTemplateType(const QUrl& url)
{
    QDir dir(url.toLocalFile());

    /*
     * Search for a description file in the url's directory.
     * If it is not found there, try cascading up a maximum of 5 directories.
     */
    int level = 0;
    while (dir.cdUp() && level < 5)
    {
        QStringList filters;
        filters << QStringLiteral("*.kdevtemplate") << QStringLiteral("*.desktop");
        foreach (const QString& entry, dir.entryList(filters))
        {
            qCDebug(PLUGIN_FILETEMPLATES) << "Trying entry" << entry;
            /*
            * This logic is not perfect, but it works for most cases.
            *
            * Project template description files usually have the suffix
            * ".kdevtemplate", so those are easy to find. For project templates,
            * all the files in the directory are template files.
            *
            * On the other hand, file templates use the generic suffix ".desktop".
            * Fortunately, those explicitly list input and output files, so we
            * only match the explicitly listed files
            */
            if (entry.endsWith(QLatin1String(".kdevtemplate")))
            {
                return ProjectTemplate;
            }

            KConfig* config = new KConfig(dir.absoluteFilePath(entry), KConfig::SimpleConfig);
            KConfigGroup group = config->group("General");

            qCDebug(PLUGIN_FILETEMPLATES) << "General group keys:" << group.keyList();

            if (!group.hasKey("Name") || !group.hasKey("Category"))
            {
                continue;
            }

            if (group.hasKey("Files"))
            {
                qCDebug(PLUGIN_FILETEMPLATES) << "Group has files " << group.readEntry("Files", QStringList());
                foreach (const QString& outputFile, group.readEntry("Files", QStringList()))
                {
                    if (dir.absoluteFilePath(config->group(outputFile).readEntry("File")) == url.toLocalFile())
                    {
                        return FileTemplate;
                    }
                }
            }

            if (group.hasKey("ShowFilesAfterGeneration"))
            {
                return ProjectTemplate;
            }
        }

        ++level;
    }

    return NoTemplate;
}

void FileTemplatesPlugin::previewTemplate()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action || !action->data().toUrl().isValid())
    {
        return;
    }
    TemplatePreviewToolView* preview = qobject_cast<TemplatePreviewToolView*>(core()->uiController()->findToolView(i18n("Template Preview"), m_toolView));
    if (!preview)
    {
        return;
    }

    core()->documentController()->activateDocument(core()->documentController()->openDocument(action->data().toUrl()));
}

#include "filetemplatesplugin.moc"
